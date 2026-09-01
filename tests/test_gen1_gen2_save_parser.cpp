#include <QtTest/QtTest>
#include <QApplication>
#include <vector>
#include <cstring>
#include <fstream>
#include "pocket/save/Gen1SaveParser.hpp"
#include "pocket/save/Gen2SaveParser.hpp"

class TestGen1Gen2SaveParser : public QObject {
    Q_OBJECT

private:
    std::string createSyntheticGen1SaveFile(const std::string& fileName, bool corrupt = false) {
        std::vector<uint8_t> buffer(32768, 0x00);

        // Player Trainer Name = "RED"
        buffer[0x2598] = 0x91; buffer[0x2599] = 0x84; buffer[0x259A] = 0x83; buffer[0x259B] = 0x50;

        // Party count = 1
        buffer[0x2F2C] = 1;
        buffer[0x2F2D] = 153; // Bulbasaur
        buffer[0x2F2E] = 0xFF;

        uint8_t* pkmn = buffer.data() + 0x2F34;
        pkmn[0x00] = 153; // Bulbasaur
        pkmn[0x21] = 5;   // Level 5

        // Stat Exp: HP 1000, Atk 2000
        pkmn[0x11] = 0x03; pkmn[0x12] = 0xE8;
        pkmn[0x13] = 0x07; pkmn[0x14] = 0xD0;

        // DVs: Atk 15, Def 10 -> dv1 = 0xFA
        // Spe 12, Spc 8 -> dv2 = 0xC8
        pkmn[0x1B] = 0xFA;
        pkmn[0x1C] = 0xC8;

        // Nickname = "BULBA"
        uint8_t* nick = buffer.data() + 0x3080;
        nick[0] = 0x81; nick[1] = 0x94; nick[2] = 0x8B; nick[3] = 0x81; nick[4] = 0x80; nick[5] = 0x50;

        // Compute Checksum
        uint8_t checksum = Pocket::Save::Gen1SaveParser::calculateChecksum(buffer.data() + 0x2598, 0x3523 - 0x2598);
        buffer[0x3523] = corrupt ? static_cast<uint8_t>(checksum ^ 0xFF) : checksum;

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 32768);
        file.close();

        return filePath;
    }

    std::string createSyntheticGen2SaveFile(const std::string& fileName, bool corruptPrimary = false, bool corruptSecondary = false) {
        std::vector<uint8_t> buffer(32768, 0x00);

        auto populateSlot = [&](size_t baseOffset) {
            // Player Trainer Name = "GOLD"
            buffer[baseOffset] = 0x87; buffer[baseOffset + 1] = 0x8E; buffer[baseOffset + 2] = 0x8B; buffer[baseOffset + 3] = 0x83; buffer[baseOffset + 4] = 0x50;

            size_t partyBase = baseOffset + 0x865;
            buffer[partyBase] = 1; // Party count = 1
            buffer[partyBase + 1] = 155; // Cyndaquil
            buffer[partyBase + 2] = 0xFF;

            uint8_t* pkmn = buffer.data() + partyBase + 8;
            pkmn[0x00] = 155; // Cyndaquil
            pkmn[0x1B] = 120; // Friendship = 120
            pkmn[0x1F] = 5;   // Level 5

            // Stat Exp
            pkmn[0x0B] = 0x03; pkmn[0x0C] = 0xE8; // HP 1000

            // DVs
            pkmn[0x15] = 0xFF; // Atk 15, Def 15
            pkmn[0x16] = 0xFF; // Spe 15, Spc 15
        };

        populateSlot(0x2000); // Primary Slot
        populateSlot(0x0C00); // Secondary Slot

        // Primary Checksum
        uint16_t priSum = Pocket::Save::Gen2SaveParser::calculateChecksum16(buffer.data() + 0x2000, 0x2B83 - 0x2000);
        if (corruptPrimary) priSum ^= 0xFFFF;
        buffer[0x2B83] = static_cast<uint8_t>((priSum >> 8) & 0xFF);
        buffer[0x2B84] = static_cast<uint8_t>(priSum & 0xFF);

        // Secondary Checksum
        uint16_t secSum = Pocket::Save::Gen2SaveParser::calculateChecksum16(buffer.data() + 0x0C00, 0x1783 - 0x0C00);
        if (corruptSecondary) secSum ^= 0xFFFF;
        buffer[0x1783] = static_cast<uint8_t>((secSum >> 8) & 0xFF);
        buffer[0x1784] = static_cast<uint8_t>(secSum & 0xFF);

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 32768);
        file.close();

        return filePath;
    }

private slots:
    void testGen1SaveParserValid() {
        std::string savePath = createSyntheticGen1SaveFile("test_gen1_valid.sav");

        Pocket::Save::Gen1SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.party.size(), static_cast<size_t>(1));

        const auto& pkmn = result.party[0];
        QCOMPARE(pkmn.generation, Pocket::Save::GenerationType::Gen1);
        QCOMPARE(pkmn.hasFriendship, false);
        QCOMPARE(pkmn.speciesId, static_cast<uint16_t>(153));
        QCOMPARE(QString::fromStdString(pkmn.speciesName), QString("Bulbasaur"));
        QCOMPARE(pkmn.level, static_cast<uint8_t>(5));
        QCOMPARE(pkmn.statExp.hp, static_cast<uint16_t>(1000));
        QCOMPARE(pkmn.statExp.attack, static_cast<uint16_t>(2000));
        QCOMPARE(pkmn.dvs.attack, static_cast<uint8_t>(15));
        QCOMPARE(pkmn.dvs.defense, static_cast<uint8_t>(10));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testGen1SaveParserChecksumMismatch() {
        std::string savePath = createSyntheticGen1SaveFile("test_gen1_corrupt.sav", true);

        Pocket::Save::Gen1SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::ChecksumFailed);

        QFile::remove(QString::fromStdString(savePath));
    }

    void testGen2SaveParserValidPrimary() {
        std::string savePath = createSyntheticGen2SaveFile("test_gen2_primary.sav");

        Pocket::Save::Gen2SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.activeSlotIndex, 0); // Primary Slot
        QCOMPARE(result.party.size(), static_cast<size_t>(1));

        const auto& pkmn = result.party[0];
        QCOMPARE(pkmn.generation, Pocket::Save::GenerationType::Gen2);
        QCOMPARE(pkmn.hasFriendship, true);
        QCOMPARE(pkmn.friendship.rawValue(), static_cast<uint8_t>(120));
        QCOMPARE(QString::fromStdString(pkmn.speciesName), QString("Cyndaquil"));
        QCOMPARE(pkmn.dvs.attack, static_cast<uint8_t>(15));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testGen2SaveParserSecondaryFallback() {
        std::string savePath = createSyntheticGen2SaveFile("test_gen2_secondary.sav", true, false);

        Pocket::Save::Gen2SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.activeSlotIndex, 1); // Secondary Backup Slot

        QFile::remove(QString::fromStdString(savePath));
    }

    void testGen2SaveParserBothSlotsCorrupt() {
        std::string savePath = createSyntheticGen2SaveFile("test_gen2_both_corrupt.sav", true, true);

        Pocket::Save::Gen2SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::ChecksumFailed);

        QFile::remove(QString::fromStdString(savePath));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestGen1Gen2SaveParser tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_gen1_gen2_save_parser.moc"
