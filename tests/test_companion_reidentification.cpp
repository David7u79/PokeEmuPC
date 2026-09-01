#include <QtTest/QtTest>
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/save/CompanionReidentifier.hpp"
#include "pocket/save/CreatureSaveParser.hpp"

class TestCompanionReidentification : public QObject {
    Q_OBJECT

private:
    Pocket::Save::Creature createDummyCreature(
        uint32_t pid,
        uint16_t speciesId,
        const std::string& speciesName,
        const std::string& nickname,
        uint8_t level,
        const std::string& location
    ) {
        Pocket::Save::Creature c;
        c.generation = Pocket::Save::GenerationType::Gen3;
        c.personalityValue = pid;
        c.speciesId = speciesId;
        c.speciesName = speciesName;
        c.nickname = nickname;
        c.level = level;
        c.location = location;

        c.trainer.trainerName = "Red";
        c.trainer.trainerId = 12345;
        c.trainer.secretId = 54321;
        c.friendship.setRawValue(70);

        return c;
    }

private slots:
    void testBoxMovementReidentification() {
        // Initial Link: Bulbasaur at Party Slot 1
        Pocket::Save::Creature initPkmn = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 1");
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash_v1");

        QCOMPARE(link.status, Pocket::Companion::LinkStatus::Linked);
        QCOMPARE(link.locator.type, Pocket::Companion::LocationType::Party);
        QCOMPARE(link.locator.partySlot, 1);

        // Updated Save: Creature moved from Party to Box 2 Slot 5
        Pocket::Save::SaveParseResult freshSave;
        freshSave.status = Pocket::Save::SaveParseStatus::Success;

        // Box 2 is index 1, Slot 5 is index 4
        freshSave.boxes.resize(14, std::vector<Pocket::Save::Creature>(30));
        freshSave.boxes[1][4] = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Box 2 Slot 5");

        Pocket::Companion::CompanionLink reidentified = Pocket::Save::CompanionReidentifier::reidentify(link, freshSave, "hash_v2");

        QCOMPARE(reidentified.status, Pocket::Companion::LinkStatus::Linked);
        QCOMPARE(reidentified.locator.type, Pocket::Companion::LocationType::Box);
        QCOMPARE(reidentified.locator.boxNumber, 2);
        QCOMPARE(reidentified.locator.boxSlot, 5);
        QCOMPARE(QString::fromStdString(reidentified.lastVerifiedSaveHash), QString("hash_v2"));
    }

    void testEvolutionReidentification() {
        // Initial Link: Bulbasaur (Species 1)
        Pocket::Save::Creature initPkmn = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 15, "Party Slot 1");
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash_v1");

        // Fresh Save: Bulbasaur evolved into Ivysaur (Species 2), same PID & OT ID
        Pocket::Save::SaveParseResult freshSave;
        freshSave.status = Pocket::Save::SaveParseStatus::Success;
        freshSave.party.push_back(createDummyCreature(100, 2, "Ivysaur", "Bulba", 16, "Party Slot 1"));

        Pocket::Companion::CompanionLink reidentified = Pocket::Save::CompanionReidentifier::reidentify(link, freshSave, "hash_v2");

        QCOMPARE(reidentified.status, Pocket::Companion::LinkStatus::Linked);
        QCOMPARE(QString::fromStdString(reidentified.speciesName), QString("Ivysaur"));
        QCOMPARE(reidentified.level, static_cast<uint8_t>(16));
    }

    void testLevelAndNicknameChangeReidentification() {
        // Initial Link: Bulbasaur Level 5, Nickname "Bulba"
        Pocket::Save::Creature initPkmn = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 1");
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash_v1");

        // Fresh Save: Renamed to "Sprout" and leveled to 25
        Pocket::Save::SaveParseResult freshSave;
        freshSave.status = Pocket::Save::SaveParseStatus::Success;
        freshSave.party.push_back(createDummyCreature(100, 1, "Bulbasaur", "Sprout", 25, "Party Slot 1"));

        Pocket::Companion::CompanionLink reidentified = Pocket::Save::CompanionReidentifier::reidentify(link, freshSave, "hash_v2");

        QCOMPARE(reidentified.status, Pocket::Companion::LinkStatus::Linked);
        QCOMPARE(QString::fromStdString(reidentified.nickname), QString("Sprout"));
        QCOMPARE(reidentified.level, static_cast<uint8_t>(25));
    }

    void testMissingCreatureHandling() {
        Pocket::Save::Creature initPkmn = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 1");
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash_v1");

        // Fresh Save: Creature released or traded away (empty save)
        Pocket::Save::SaveParseResult emptySave;
        emptySave.status = Pocket::Save::SaveParseStatus::Success;

        Pocket::Companion::CompanionLink reidentified = Pocket::Save::CompanionReidentifier::reidentify(link, emptySave, "hash_v2");

        QCOMPARE(reidentified.status, Pocket::Companion::LinkStatus::NotFound);
    }

    void testAmbiguousMatchDetection() {
        Pocket::Save::Creature initPkmn = createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 1");
        Pocket::Companion::CompanionLink link = Pocket::Save::CompanionReidentifier::createLinkFromCreature(initPkmn, 1, "hash_v1");

        // Fresh Save: Two identical PIDs (cloned creature in Party Slot 1 and Slot 2)
        Pocket::Save::SaveParseResult ambiguousSave;
        ambiguousSave.status = Pocket::Save::SaveParseStatus::Success;
        ambiguousSave.party.push_back(createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 1"));
        ambiguousSave.party.push_back(createDummyCreature(100, 1, "Bulbasaur", "Bulba", 5, "Party Slot 2"));

        Pocket::Companion::CompanionLink reidentified = Pocket::Save::CompanionReidentifier::reidentify(link, ambiguousSave, "hash_v2");

        QCOMPARE(reidentified.status, Pocket::Companion::LinkStatus::AmbiguousMatch);
    }
};

QTEST_MAIN(TestCompanionReidentification)
#include "test_companion_reidentification.moc"
