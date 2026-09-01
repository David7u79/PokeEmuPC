#include <QtTest/QtTest>
#include "pocket/core/GameSystem.hpp"

class TestGameSystem : public QObject {
    Q_OBJECT
private slots:
    void testExtensionDetection() {
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("pokemon_red.gb"), Pocket::Core::GameSystem::GB);
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("pokemon_crystal.gbc"), Pocket::Core::GameSystem::GBC);
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("pokemon_emerald.gba"), Pocket::Core::GameSystem::GBA);
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("pokemon_platinum.nds"), Pocket::Core::GameSystem::NDS);

        // Case insensitivity
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("ROM.GBA"), Pocket::Core::GameSystem::GBA);
        QCOMPARE(Pocket::Core::GameSystemUtils::detectFromExtension("GAME.NDS"), Pocket::Core::GameSystem::NDS);

        // Invalid extension
        QVERIFY(!Pocket::Core::GameSystemUtils::detectFromExtension("invalid.txt").has_value());
    }

    void testStringConversions() {
        QCOMPARE(Pocket::Core::GameSystemUtils::toString(Pocket::Core::GameSystem::GBA), std::string("GBA"));
        QCOMPARE(Pocket::Core::GameSystemUtils::fromString("GBA"), Pocket::Core::GameSystem::GBA);
        QCOMPARE(Pocket::Core::GameSystemUtils::fromString("gba"), Pocket::Core::GameSystem::GBA);
        QCOMPARE(Pocket::Core::GameSystemUtils::fromString("unknown"), Pocket::Core::GameSystem::Unknown);
    }
};

QTEST_MAIN(TestGameSystem)
#include "test_game_system.moc"
