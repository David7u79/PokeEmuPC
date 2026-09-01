#include <QtTest/QtTest>
#include <QApplication>
#include <vector>
#include <cstring>
#include <fstream>
#include "pocket/save/Gen4SaveParser.hpp"
#include "pocket/save/Gen5SaveParser.hpp"
#include "pocket/save/CompanionReidentifier.hpp"

class TestGen4Gen5SaveParser : public QObject {
    Q_OBJECT

private:
    std::string createSyntheticGen4SaveFile(const std::string& fileName) {
        std::vector<uint8_t> buffer(524288, 0x00);

        // Small Block Slot A Counter
        *reinterpret_cast<uint32_t*>(buffer.data() + 0xC0F0) = 15;

        // Party count = 1 at offset 0x94
        *reinterpret_cast<uint32_t*>(buffer.data() + 0x94) = 1;

        // Build 236-byte Gen 4 struct at 0x98
        uint8_t* pkmn = buffer.data() + 0x98;

        uint32_t pid = 0;
        uint16_t checksum = 12345;

        uint8_t unenc[128]{};
        uint8_t* blockA = unenc + 0x00;
        uint8_t* blockB = unenc + 0x20;
        uint8_t* blockC = unenc + 0x40; // Nickname
        uint8_t* blockD = unenc + 0x60; // OT Name

        *reinterpret_cast<uint16_t*>(blockA + 0x00) = 387; // Turtwig
        *reinterpret_cast<uint32_t*>(blockA + 0x08) = 5000;
        blockA[0x0C] = 100; // Friendship

        // Nickname "TURTWIG" in UTF-16 LE
        uint16_t nickU16[] = {'T','U','R','T','W','I','G', 0x0000};
        std::memcpy(blockC, nickU16, sizeof(nickU16));

        // OT Name "DAWN" in UTF-16 LE
        uint16_t otU16[] = {'D','A','W','N', 0x0000};
        std::memcpy(blockD, otU16, sizeof(otU16));

        blockB[0x04] = 12; // HP EV
        blockB[0x05] = 24; // Atk EV

        // Encrypt & Shuffle into raw struct (236 bytes)
        uint8_t data236[236]{};
        *reinterpret_cast<uint32_t*>(data236 + 0x00) = pid;
        *reinterpret_cast<uint16_t*>(data236 + 0x06) = checksum;

        std::memcpy(data236 + 0x08, unenc, 128);

        uint32_t seed = checksum;
        uint16_t* words = reinterpret_cast<uint16_t*>(data236 + 0x08);
        for (int i = 0; i < 64; ++i) {
            seed = seed * 0x41C64E6D + 0x60B90885;
            uint16_t key = static_cast<uint16_t>((seed >> 16) & 0xFFFF);
            words[i] ^= key;
        }

        data236[0x8C] = 12; // Level 12
        std::memcpy(pkmn, data236, 236);

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 524288);
        file.close();

        return filePath;
    }

    std::string createSyntheticGen5SaveFile(const std::string& fileName) {
        std::vector<uint8_t> buffer(524288, 0x00);

        // Party count = 1 at offset 0x18E04
        *reinterpret_cast<uint32_t*>(buffer.data() + 0x18E04) = 1;

        // Build 220-byte Gen 5 struct at 0x18E08
        uint8_t* pkmn = buffer.data() + 0x18E08;
        uint32_t pid = 0;
        uint16_t checksum = 54321;

        uint8_t unenc[128]{};
        uint8_t* blockA = unenc + 0x00;
        uint8_t* blockC = unenc + 0x40; // Nickname

        *reinterpret_cast<uint16_t*>(blockA + 0x00) = 495; // Snivy

        // Nickname "SNIVY"
        uint16_t nickU16[] = {'S','N','I','V','Y', 0x0000};
        std::memcpy(blockC, nickU16, sizeof(nickU16));

        // Encrypt (220 bytes)
        uint8_t data220[220]{};
        *reinterpret_cast<uint32_t*>(data220 + 0x00) = pid;
        *reinterpret_cast<uint16_t*>(data220 + 0x06) = checksum;
        std::memcpy(data220 + 0x08, unenc, 128);

        uint32_t seed = checksum;
        uint16_t* words = reinterpret_cast<uint16_t*>(data220 + 0x08);
        for (int i = 0; i < 64; ++i) {
            seed = seed * 0x41C64E6D + 0x60B90885;
            uint16_t key = static_cast<uint16_t>((seed >> 16) & 0xFFFF);
            words[i] ^= key;
        }

        data220[0x8C] = 5; // Level 5
        std::memcpy(pkmn, data220, 220);

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 524288);
        file.close();

        return filePath;
    }

private slots:
    void testGen4SaveParserValid() {
        std::string savePath = createSyntheticGen4SaveFile("test_gen4.sav");

        Pocket::Save::Gen4SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.party.size(), static_cast<size_t>(1));

        const auto& pkmn = result.party[0];
        QCOMPARE(pkmn.generation, Pocket::Save::GenerationType::Gen4);
        QCOMPARE(pkmn.speciesId, static_cast<uint16_t>(387));
        QCOMPARE(QString::fromStdString(pkmn.speciesName), QString("Turtwig"));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testGen5SaveParserValid() {
        std::string savePath = createSyntheticGen5SaveFile("test_gen5.sav");

        Pocket::Save::Gen5SaveParser parser;
        Pocket::Save::SaveParseResult result = parser.parseSaveFile(savePath);

        QCOMPARE(result.status, Pocket::Save::SaveParseStatus::Success);
        QCOMPARE(result.party.size(), static_cast<size_t>(1));

        const auto& pkmn = result.party[0];
        QCOMPARE(pkmn.generation, Pocket::Save::GenerationType::Gen5);
        QCOMPARE(pkmn.speciesId, static_cast<uint16_t>(495));
        QCOMPARE(QString::fromStdString(pkmn.speciesName), QString("Snivy"));

        QFile::remove(QString::fromStdString(savePath));
    }

    void testCompanionReidentificationAcrossEvolution() {
        // Initial Turtwig at Level 12 in Party Slot 1
        Pocket::Save::Creature initPkmn;
        initPkmn.generation = Pocket::Save::GenerationType::Gen4;
        initPkmn.personalityValue = 5555;
        initPkmn.trainer.trainerId = 12345;
        initPkmn.trainer.secretId = 54321;
        initPkmn.speciesId = 387;
        initPkmn.speciesName = "Turtwig";
        initPkmn.nickname = "Turtwig";
        initPkmn.level = 12;
        initPkmn.location = "Party Slot 1";
        initPkmn.friendship.setRawValue(100);

        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash1");

        // Evolved Grotle at Level 18 moved to PC Box 2 Slot 10
        Pocket::Save::SaveParseResult freshSave;
        freshSave.status = Pocket::Save::SaveParseStatus::Success;
        freshSave.boxes.resize(18);
        freshSave.boxes[1].resize(30);

        Pocket::Save::Creature evolvedPkmn = initPkmn;
        evolvedPkmn.speciesId = 388;
        evolvedPkmn.speciesName = "Grotle";
        evolvedPkmn.level = 18;
        evolvedPkmn.location = "Box 2 Slot 10";

        freshSave.boxes[1][9] = evolvedPkmn; // Box 2 Slot 10

        // Reidentify
        Pocket::Companion::CompanionLink relinked = Pocket::Save::CompanionReidentifier::reidentify(link, freshSave, "hash2");

        QCOMPARE(relinked.status, Pocket::Companion::LinkStatus::Linked);
        QCOMPARE(relinked.locator.type, Pocket::Companion::LocationType::Box);
        QCOMPARE(relinked.locator.boxNumber, 2);
        QCOMPARE(relinked.locator.boxSlot, 10);
        QCOMPARE(QString::fromStdString(relinked.speciesName), QString("Grotle"));
        QCOMPARE(relinked.level, static_cast<uint8_t>(18));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestGen4Gen5SaveParser tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_gen4_gen5_save_parser.moc"
