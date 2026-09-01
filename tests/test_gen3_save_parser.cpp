#include <QtTest/QtTest>
#include <vector>
#include <cstring>
#include "pocket/save/Gen3SaveParser.hpp"

class TestGen3SaveParser : public QObject {
    Q_OBJECT

private:
    std::vector<uint8_t> createSyntheticGen3SaveBuffer(uint32_t slotACounter, uint32_t slotBCounter) {
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

                // "Bulba" encoded: 'B' = 0xBC, 'u' = 0xE9, 'l' = 0xE0, 'b' = 0xD6, 'a' = 0xD5, End = 0xFF
                pkmnPtr[0x08] = 0xBC; pkmnPtr[0x09] = 0xE9; pkmnPtr[0x0A] = 0xE0;
                pkmnPtr[0x0B] = 0xD6; pkmnPtr[0x0C] = 0xD5; pkmnPtr[0x0D] = 0xFF;

                pkmnPtr[0x14] = 0xCC; pkmnPtr[0x15] = 0xD9; pkmnPtr[0x16] = 0xD8; pkmnPtr[0x17] = 0xFF;

                uint8_t blockG[12]{};
                *reinterpret_cast<uint16_t*>(blockG + 0) = 1; // Bulbasaur
                *reinterpret_cast<uint32_t*>(blockG + 4) = 125;
                blockG[9] = 70;

                uint8_t blockA[12]{};
                uint8_t blockE[12]{};
                blockE[0] = 10;
                blockE[1] = 20;
                blockE[2] = 30;

                uint8_t blockM[12]{};

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
            populateSection(buffer.data() + (sec * 4096), sec, slotACounter);
        }

        for (uint16_t sec = 0; sec < 14; ++sec) {
            populateSection(buffer.data() + 0x0E000 + (sec * 4096), sec, slotBCounter);
        }

        return buffer;
    }

private slots:
    void testSyntheticGen3SaveParsing() {
        std::vector<uint8_t> buffer = createSyntheticGen3SaveBuffer(10, 5);

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveBuffer(buffer);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.activeSlotIndex, 0);
        QCOMPARE(result.saveCounter, static_cast<uint32_t>(10));
        QCOMPARE(QString::fromStdString(result.trainerName), QString("Red"));
        QCOMPARE(result.trainerId, static_cast<uint16_t>(12345));

        QCOMPARE(result.party.size(), static_cast<size_t>(1));
        const auto& pkmn = result.party[0];

        QCOMPARE(pkmn.speciesId, static_cast<uint16_t>(1));
        QCOMPARE(QString::fromStdString(pkmn.speciesName), QString("Bulbasaur"));
        QCOMPARE(QString::fromStdString(pkmn.nickname), QString("Bulba"));
        QCOMPARE(pkmn.level, static_cast<uint8_t>(5));
        QCOMPARE(pkmn.evs.hp, static_cast<uint8_t>(10));
        QCOMPARE(pkmn.evs.attack, static_cast<uint8_t>(20));
        QCOMPARE(pkmn.evs.defense, static_cast<uint8_t>(30));
        QCOMPARE(pkmn.friendship.rawValue(), static_cast<uint8_t>(70));
    }

    void testHigherSaveCounterSlotSelection() {
        std::vector<uint8_t> buffer = createSyntheticGen3SaveBuffer(10, 25);

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveBuffer(buffer);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.activeSlotIndex, 1);
        QCOMPARE(result.saveCounter, static_cast<uint32_t>(25));
    }

    void testCorruptedChecksumRejection() {
        std::vector<uint8_t> buffer = createSyntheticGen3SaveBuffer(10, 5);

        buffer[0xFF6] = 0xDE; buffer[0xFF7] = 0xAD;
        buffer[0x0E000 + 0xFF6] = 0xDE; buffer[0x0E000 + 0xFF7] = 0xAD;

        Pocket::Save::Gen3SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveBuffer(buffer);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::NoValidSlotFound);
    }
};

QTEST_MAIN(TestGen3SaveParser)
#include "test_gen3_save_parser.moc"
