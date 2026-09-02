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
            engine.stop();

            const PersistentGameSave produced = engine.getPersistentSave();
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

            if (status == SaveParseStatus::Success) {
                QVERIFY2(!result.trainerName.empty(), "trainer name did not survive the session");
                QVERIFY2(result.activeSlotIndex >= 0, "no valid save slot after emulation");
                QVERIFY2(!result.party.empty(), "party came back empty");
                for (const auto& creature : result.party) {
                    QVERIFY2(creature.speciesId > 0, "party member has no species");
                }
            }
        }

        qInfo().noquote() << "gen4 parse status :" << static_cast<int>(status);
        qInfo().noquote() << "trainer           :" << QString::fromStdString(trainerName);
        qInfo().noquote() << "party size        :" << partySize;

        // Whatever the parser concluded, the developer's file is untouched.
        QCOMPARE(sha256Of(m_sourceSave), sourceHashBefore);

        QVERIFY2(status == SaveParseStatus::Success,
                 qPrintable(QString("Gen4SaveParser rejected the emulated save (status %1). A save written "
                                    "before the game itself saved in-game will not parse.")
                                .arg(static_cast<int>(status))));
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
