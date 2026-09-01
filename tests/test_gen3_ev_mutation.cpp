#include <QtTest/QtTest>
#include <QApplication>
#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>
#include "pocket/save/Gen3SaveEditor.hpp"
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/save/CompanionReidentifier.hpp"

class TestGen3EvMutation : public QObject {
    Q_OBJECT

private:
    std::string createSyntheticGen3SaveFile(const std::string& fileName) {
        std::vector<uint8_t> buffer(131072, 0x00);

        auto populateSection = [&](uint8_t* secPtr, uint16_t sectionId, uint32_t saveCounter) {
            std::memset(secPtr, 0x00, 4096);

            if (sectionId == 0) {
                secPtr[0x00] = 0xCC; secPtr[0x01] = 0xD9; secPtr[0x02] = 0xD8; secPtr[0x03] = 0xFF;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0A) = 12345;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0C) = 54321;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0E) = 12;
                secPtr[0x10] = 34;
            } else if (sectionId == 1) {
                *reinterpret_cast<uint32_t*>(secPtr + 0x234) = 1; // Party count = 1

                uint8_t* pkmnPtr = secPtr + 0x238;
                uint32_t pid = 100;
                uint32_t otId = 12345;

                *reinterpret_cast<uint32_t*>(pkmnPtr + 0x00) = pid;
                *reinterpret_cast<uint32_t*>(pkmnPtr + 0x04) = otId;

                // "Treeck"
                pkmnPtr[0x08] = 0xD9; pkmnPtr[0x09] = 0xE7; pkmnPtr[0x0A] = 0xE3;
                pkmnPtr[0x0B] = 0xD7; pkmnPtr[0x0C] = 0xD9; pkmnPtr[0x0D] = 0xFF;

                // "Red"
                pkmnPtr[0x14] = 0xCC; pkmnPtr[0x15] = 0xD9; pkmnPtr[0x16] = 0xD8; pkmnPtr[0x17] = 0xFF;

                uint8_t blockG[12]{};
                *reinterpret_cast<uint16_t*>(blockG + 0) = 252; // Treecko
                *reinterpret_cast<uint32_t*>(blockG + 4) = 125;
                blockG[9] = 70;

                uint8_t blockA[12]{};

                // Block E: Baseline EVs = 0
                uint8_t blockE[12]{};
                blockE[0] = 0; // HP EV
                blockE[1] = 0; // Atk EV
                blockE[2] = 0; // Def EV
                blockE[3] = 0; // Spe EV
                blockE[4] = 0; // SpA EV
                blockE[5] = 0; // SpD EV

                uint8_t blockM[12]{};

                // PID = 100 -> GMAE order
                std::memcpy(pkmnPtr + 0x20, blockG, 12);
                std::memcpy(pkmnPtr + 0x2C, blockM, 12);
                std::memcpy(pkmnPtr + 0x38, blockA, 12);
                std::memcpy(pkmnPtr + 0x44, blockE, 12);

                uint32_t key = pid ^ otId;
                uint32_t* dwords = reinterpret_cast<uint32_t*>(pkmnPtr + 0x20);
                for (int d = 0; d < 12; ++d) {
                    dwords[d] ^= key;
                }

                pkmnPtr[0x54] = 5;
            }

            *reinterpret_cast<uint16_t*>(secPtr + 0xFF4) = sectionId;
            *reinterpret_cast<uint32_t*>(secPtr + 0xFF8) = 0x08012002;
            *reinterpret_cast<uint32_t*>(secPtr + 0xFFC) = saveCounter;

            uint16_t checksum = Pocket::Save::Gen3SaveParser::calculateSectionChecksum(secPtr);
            *reinterpret_cast<uint16_t*>(secPtr + 0xFF6) = checksum;
        };

        for (uint16_t sec = 0; sec < 14; ++sec) {
            populateSection(buffer.data() + (sec * 4096), sec, 10);
            populateSection(buffer.data() + 0x0E000 + (sec * 4096), sec, 5);
        }

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 131072);
        file.close();

        return filePath;
    }

private slots:
    void testAllSixStatsMutation() {
        std::string savePath = createSyntheticGen3SaveFile("test_ev_stats.sav");

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult origParse = parser.parseSaveFile(savePath);
        QCOMPARE(origParse.status, Pocket::Save::SaveParseStatus::Success);

        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(origParse.party[0], 1, "orig_hash");

        auto coord = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        auto backupRepo = std::make_shared<Pocket::Save::SaveBackupRepository>();
        Pocket::Save::Gen3SaveEditor editor(coord, backupRepo);

        // 1. Mutate Speed EV (+4)
        Pocket::Save::MutationResult r1 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Speed, 4);
        if (r1.status != Pocket::Save::EditorStatus::Success) {
            qWarning() << "r1 failed:" << QString::fromStdString(r1.errorMessage);
        }
        QCOMPARE(r1.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(r1.audit.appliedEvAmount, 4);
        QCOMPARE(r1.audit.newEvValue, static_cast<uint8_t>(4));
        QVERIFY(r1.audit.bytesModified >= 2 && r1.audit.bytesModified <= 3); // 1 EV byte + 1-2 checksum bytes

        // 2. Mutate Attack EV (+4)
        Pocket::Save::MutationResult r2 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Attack, 4);
        if (r2.status != Pocket::Save::EditorStatus::Success) {
            qWarning() << "r2 failed:" << QString::fromStdString(r2.errorMessage);
        }
        QCOMPARE(r2.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(r2.audit.appliedEvAmount, 4);
        QCOMPARE(r2.audit.newEvValue, static_cast<uint8_t>(4));

        // Re-parse and verify mutated EVs in save file
        Pocket::Save::SaveParseResult modParse = parser.parseSaveFile(savePath);
        QCOMPARE(modParse.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(modParse.party[0].evs.speed, static_cast<uint8_t>(4));
        QCOMPARE(modParse.party[0].evs.attack, static_cast<uint8_t>(4));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testPerStatCap252AndPartialApplication() {
        std::string savePath = createSyntheticGen3SaveFile("test_ev_cap.sav");

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult origParse = parser.parseSaveFile(savePath);
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(origParse.party[0], 1, "orig_hash");

        auto coord = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        auto backupRepo = std::make_shared<Pocket::Save::SaveBackupRepository>();
        Pocket::Save::Gen3SaveEditor editor(coord, backupRepo);

        // Add 250 Speed EV
        Pocket::Save::MutationResult r1 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Speed, 250);
        if (r1.status != Pocket::Save::EditorStatus::Success) {
            qWarning() << "cap r1 failed:" << QString::fromStdString(r1.errorMessage);
        }
        QCOMPARE(r1.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(r1.audit.appliedEvAmount, 250);

        // Request +10 Speed EV when stat is at 250 (Cap 252 -> should apply +2, remaining +8)
        Pocket::Save::MutationResult r2 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Speed, 10);
        if (r2.status != Pocket::Save::EditorStatus::Success) {
            qWarning() << "cap r2 failed:" << QString::fromStdString(r2.errorMessage);
        }
        QCOMPARE(r2.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(r2.audit.requestedEvAmount, 10);
        QCOMPARE(r2.audit.appliedEvAmount, 2);
        QCOMPARE(r2.audit.remainingEvAmount, 8);
        QCOMPARE(r2.audit.newEvValue, static_cast<uint8_t>(252));

        // Request +4 Speed EV when stat is at 252 cap -> should reject with CapExceededNoGain
        Pocket::Save::MutationResult r3 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Speed, 4);
        QCOMPARE(r3.status, Pocket::Save::EditorStatus::CapExceededNoGain);
        QCOMPARE(r3.audit.appliedEvAmount, 0);

        QFile::remove(QString::fromStdString(savePath));
    }

    void testTotalEvCap510() {
        std::string savePath = createSyntheticGen3SaveFile("test_total_ev_cap.sav");

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult origParse = parser.parseSaveFile(savePath);
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(origParse.party[0], 1, "orig_hash");

        auto coord = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        auto backupRepo = std::make_shared<Pocket::Save::SaveBackupRepository>();
        Pocket::Save::Gen3SaveEditor editor(coord, backupRepo);

        // Max out 2 stats: 252 Atk + 252 Speed = 504 total EVs
        editor.mutateEV(savePath, link, Pocket::Save::EVType::Attack, 252);
        editor.mutateEV(savePath, link, Pocket::Save::EVType::Speed, 252);

        // Request +10 HP EV when total is 504 (Total Cap 510 -> should apply +6, remaining +4)
        Pocket::Save::MutationResult r3 = editor.mutateEV(savePath, link, Pocket::Save::EVType::HP, 10);
        if (r3.status != Pocket::Save::EditorStatus::Success) {
            qWarning() << "total cap r3 failed:" << QString::fromStdString(r3.errorMessage);
        }
        QCOMPARE(r3.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(r3.audit.appliedEvAmount, 6);
        QCOMPARE(r3.audit.remainingEvAmount, 4);
        QCOMPARE(r3.audit.newEvValue, static_cast<uint8_t>(6));

        // Total is now 510! Any further EV gain must be rejected
        Pocket::Save::MutationResult r4 = editor.mutateEV(savePath, link, Pocket::Save::EVType::Defense, 4);
        QCOMPARE(r4.status, Pocket::Save::EditorStatus::CapExceededNoGain);

        QFile::remove(QString::fromStdString(savePath));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestGen3EvMutation tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_gen3_ev_mutation.moc"
