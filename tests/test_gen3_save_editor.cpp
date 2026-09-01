#include <QtTest/QtTest>
#include <QApplication>
#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>
#include "pocket/save/Gen3SaveEditor.hpp"
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/save/CompanionReidentifier.hpp"

class TestGen3SaveEditor : public QObject {
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

                // "Bulba"
                pkmnPtr[0x08] = 0xBC; pkmnPtr[0x09] = 0xE9; pkmnPtr[0x0A] = 0xE0;
                pkmnPtr[0x0B] = 0xD6; pkmnPtr[0x0C] = 0xD5; pkmnPtr[0x0D] = 0xFF;

                // "Red"
                pkmnPtr[0x14] = 0xCC; pkmnPtr[0x15] = 0xD9; pkmnPtr[0x16] = 0xD8; pkmnPtr[0x17] = 0xFF;

                uint8_t blockG[12]{};
                *reinterpret_cast<uint16_t*>(blockG + 0) = 1; // Bulbasaur
                *reinterpret_cast<uint32_t*>(blockG + 4) = 125;
                blockG[9] = 70; // Baseline Friendship = 70

                uint8_t blockA[12]{};
                uint8_t blockE[12]{};
                blockE[0] = 10; blockE[1] = 20; blockE[2] = 30;
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
    void testExact3ByteBinaryDiff() {
        std::string savePath = createSyntheticGen3SaveFile("test_friendship_diff.sav");

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult origParse = parser.parseSaveFile(savePath);
        QCOMPARE(origParse.status, Pocket::Save::SaveParseStatus::Success);

        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(origParse.party[0], 1, "orig_hash");

        auto coord = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        auto backupRepo = std::make_shared<Pocket::Save::SaveBackupRepository>();
        Pocket::Save::Gen3SaveEditor editor(coord, backupRepo);

        // Mutate Friendship: 70 -> 100
        Pocket::Save::MutationResult result = editor.mutateFriendship(savePath, link, 100);

        if (result.status != Pocket::Save::EditorStatus::Success) {
            std::cout << "Mutation failed: " << result.errorMessage << std::endl;
        }

        QCOMPARE(result.status, Pocket::Save::EditorStatus::Success);
        QCOMPARE(result.audit.oldFriendship, static_cast<uint8_t>(70));
        QCOMPARE(result.audit.newFriendship, static_cast<uint8_t>(100));
        QVERIFY(result.audit.bytesModified >= 2 && result.audit.bytesModified <= 3); // Friendship byte + checksum byte(s)
        QCOMPARE(result.audit.unrelatedFieldsChanged, static_cast<size_t>(0));

        // Re-parse and verify mutated save
        Pocket::Save::SaveParseResult modParse = parser.parseSaveFile(savePath);
        QCOMPARE(modParse.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(modParse.party[0].friendship.rawValue(), static_cast<uint8_t>(100));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testEmulatorConcurrencyLock() {
        std::string savePath = createSyntheticGen3SaveFile("test_emulator_lock.sav");

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult origParse = parser.parseSaveFile(savePath);
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(origParse.party[0], 1, "orig_hash");

        auto coord = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        auto backupRepo = std::make_shared<Pocket::Save::SaveBackupRepository>();
        Pocket::Save::Gen3SaveEditor editor(coord, backupRepo);

        // Lock save with active emulator
        coord->acquireEmulatorLock(savePath);

        Pocket::Save::MutationResult result = editor.mutateFriendship(savePath, link, 120);

        QCOMPARE(result.status, Pocket::Save::EditorStatus::SaveLockedByEmulator);

        // Verify save on disk remains 100% untouched
        Pocket::Save::SaveParseResult modParse = parser.parseSaveFile(savePath);
        QCOMPARE(modParse.party[0].friendship.rawValue(), static_cast<uint8_t>(70));

        coord->releaseEmulatorLock(savePath);
        QFile::remove(QString::fromStdString(savePath));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestGen3SaveEditor tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_gen3_save_editor.moc"
