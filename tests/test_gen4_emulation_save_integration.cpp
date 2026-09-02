// The full chain the milestone is really about:
//
//   developer ROM + a copy of the developer save
//     -> MelonDsEngine -> melonDS DS -> retro_run -> clean shutdown
//     -> persistent save -> Gen4SaveParser
//
// Nothing here fabricates a save. The whole point is that a real emulator
// session produced the bytes, so when no Gen IV save exists yet the test skips
// and says so rather than inventing a fixture that would prove nothing.
//
// The developer's files are immutable inputs: the ROM is only ever read, and the
// save is copied into a temporary workspace before the emulator touches it.

#include <QtTest/QtTest>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <memory>
#include "DevAssets.hpp"
#include "pocket/emulator/MelonDsEngine.hpp"
#include "pocket/save/Gen4SaveParser.hpp"
#include "pocket/save/SaveSessionCoordinator.hpp"

using Pocket::Emulator::MelonDsEngine;
using Pocket::Emulator::PersistentGameSave;
using Pocket::Save::Gen4SaveParser;
using Pocket::Save::SaveParseStatus;
using Pocket::Save::SaveSessionCoordinator;
using Pocket::Save::SaveSessionState;

namespace {

QByteArray sha256Of(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    return hash.result();
}

} // namespace

class TestGen4EmulationSaveIntegration : public QObject {
    Q_OBJECT

private:
    QString m_core, m_rom, m_sourceSave;

    // Returns false and QSKIPs the caller when the fixture is not available.
    bool resolveFixture()
    {
        m_core = DevAssets::melonDsCore();
        const auto pair = DevAssets::gen4RomAndSave();
        m_rom = pair.first;
        m_sourceSave = pair.second;
        return !m_core.isEmpty() && !m_rom.isEmpty() && !m_sourceSave.isEmpty();
    }

private slots:
    void emulatedSessionProducesAParsableGen4Save()
    {
        if (!resolveFixture()) {
            QSKIP("No developer-local Gen IV save fixture found: need a Gen IV ROM "
                  "(game code ADA/APA/CPU/IPK/IPG) with a matching .nds.sav beside it, "
                  "or POCKET_GEN4_ROM and POCKET_GEN4_SAVE");
        }

        // The developer's save must come out of this byte-identical.
        const QByteArray sourceHashBefore = sha256Of(m_sourceSave);
        QVERIFY(!sourceHashBefore.isEmpty());

        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const QString workingSave = workspace.filePath("session.sav");
        QVERIFY2(QFile::copy(m_sourceSave, workingSave), "could not copy the save into the workspace");

        PersistentGameSave seed;
        QVERIFY(seed.loadFromFile(workingSave.toStdString()));
        QVERIFY(!seed.isEmpty());

        auto coordinator = std::make_shared<SaveSessionCoordinator>();
        SaveParseStatus status = SaveParseStatus::NoValidSlotFound;
        std::string trainerName;
        size_t partySize = 0;
        size_t boxed = 0;
        bool startedGame = false;
        {
            MelonDsEngine engine(m_core.toStdString(), coordinator);
            QVERIFY2(engine.hasCore(), engine.coreError().c_str());
            QVERIFY2(engine.loadRom(m_rom.toStdString()), "core rejected the Gen IV ROM");

            // Ownership is the emulator's for the whole session.
            const std::string lockedPath = engine.saveFilePath();
            QVERIFY(!lockedPath.empty());
            QCOMPARE(coordinator->getState(lockedPath), SaveSessionState::EmulatorActive);
            QVERIFY(!coordinator->canMutateSave(lockedPath));

            QVERIFY(engine.loadPersistentSave(seed));

            std::atomic<int> frames{0};
            engine.setVideoFrameCallback([&frames](const uint8_t*, int, int, size_t) { ++frames; });
            engine.start();
            QTRY_VERIFY_WITH_TIMEOUT(frames.load() > 120, 10000);

            // Read before stop(): unloading the game takes save RAM away with it.
            const PersistentGameSave produced = engine.getPersistentSave();
            engine.stop();

            QVERIFY2(!produced.isEmpty(), "the session produced no save RAM");
            QVERIFY(produced.saveToFile(workingSave.toStdString()));

            // Released on stop, so canonical companion mutations may proceed.
            QCOMPARE(coordinator->getState(lockedPath), SaveSessionState::Available);
            QVERIFY(coordinator->canMutateSave(lockedPath));

            Gen4SaveParser parser;
            const auto result = parser.parseSaveFile(workingSave.toStdString());
            status = result.status;
            trainerName = result.trainerName;
            partySize = result.party.size();

            // Report before asserting, so a failure says what was actually read.
            qInfo().noquote() << "gen4 parse status :" << static_cast<int>(status) << result.errorMessage.c_str();
            qInfo().noquote() << "active slot       :" << result.activeSlotIndex << "counter" << result.saveCounter;
            qInfo().noquote() << "trainer           :" << QString::fromStdString(result.trainerName)
                              << "id" << result.trainerId << "money" << result.money;
            qInfo().noquote() << "play time         :" << result.playTimeHours << "h" << result.playTimeMinutes << "m";
            qInfo().noquote() << "party size        :" << result.party.size();
            for (const auto& creature : result.party) {
                qInfo().noquote() << "  party member    : species" << creature.speciesId << "level"
                                  << creature.level;
            }
            for (const auto& box : result.boxes) boxed += box.size();
            qInfo().noquote() << "boxed creatures   :" << boxed;

            // A blank cartridge still parses: the structure is there, the content
            // is not. Trainer id, money, play time and party all reading zero means
            // nothing was ever saved from inside the game, and asserting against
            // that would be a test that passes while proving nothing.
            startedGame = result.trainerId != 0 || result.money != 0 || result.playTimeHours != 0 ||
                          result.playTimeMinutes != 0 || !result.party.empty() || boxed != 0;

            if (status == SaveParseStatus::Success && startedGame) {
                QVERIFY2(!result.trainerName.empty(), "trainer name did not survive the session");
                QVERIFY2(result.activeSlotIndex >= 0, "no valid save slot after emulation");
                // The party may still be empty before the starter is handed over.
                for (const auto& creature : result.party) {
                    QVERIFY2(creature.speciesId > 0, "party member has no species");
                    QVERIFY2(creature.level > 0, "party member has no level");
                }
            }
        }

        // Whatever the parser concluded, the developer's file is untouched.
        QCOMPARE(sha256Of(m_sourceSave), sourceHashBefore);

        QVERIFY2(status == SaveParseStatus::Success,
                 qPrintable(QString("Gen4SaveParser rejected the emulated save (status %1).")
                                .arg(static_cast<int>(status))));

        if (!startedGame) {
            QSKIP("The save parses but holds no started game: trainer id, money, play time, party and "
                  "boxes are all empty. Play far enough to be handed a starter, save from inside the "
                  "game, then close PocketPartner so a real save is written.");
        }
        QVERIFY2(partySize > 0, "a started game reported an empty party");
    }

    void theSourceSaveIsNeverOpenedForWriting()
    {
        if (!resolveFixture()) {
            QSKIP("No developer-local Gen IV save fixture found");
        }

        // A guard against a future refactor pointing the engine at the original.
        const QByteArray before = sha256Of(m_sourceSave);
        auto coordinator = std::make_shared<SaveSessionCoordinator>();
        MelonDsEngine engine(m_core.toStdString(), coordinator);
        QVERIFY(engine.hasCore());
        QVERIFY(engine.loadRom(m_rom.toStdString()));
        engine.stop();
        QCOMPARE(sha256Of(m_sourceSave), before);
    }
};

QTEST_MAIN(TestGen4EmulationSaveIntegration)
#include "test_gen4_emulation_save_integration.moc"
