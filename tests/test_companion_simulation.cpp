#include <QtTest/QtTest>
#include <memory>
#include "pocket/companion/IClock.hpp"
#include "pocket/companion/CompanionState.hpp"
#include "pocket/companion/CompanionSimulator.hpp"

class TestCompanionSimulation : public QObject {
    Q_OBJECT
private slots:
    void testOneHourElapsedDecay() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState initialState;
        initialState.lastCalculatedAt = 100000;
        initialState.hunger = 100.0;
        initialState.energy = 100.0;
        initialState.mood   = 100.0;
        initialState.fatigue= 0.0;

        // Advance clock by 1 hour (3600s)
        testClock->advanceHours(1);

        Pocket::Companion::CompanionState state = simulator.calculateCurrentState(initialState);

        QCOMPARE(state.lastCalculatedAt, static_cast<int64_t>(100000 + 3600));
        QCOMPARE(state.hunger, 96.0); // 100 - 4
        QCOMPARE(state.energy, 97.5); // 100 - 2.5
        QCOMPARE(state.mood, 97.0);   // 100 - 3
        QCOMPARE(state.fatigue, 2.0); // 0 + 2
    }

    void testEightHoursElapsedDecay() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState initialState;
        initialState.lastCalculatedAt = 100000;
        initialState.hunger = 100.0;
        initialState.energy = 100.0;
        initialState.mood   = 100.0;

        // Advance clock by 8 hours (simulating app closed overnight)
        testClock->advanceHours(8);

        Pocket::Companion::CompanionState state = simulator.calculateCurrentState(initialState);

        QCOMPARE(state.hunger, 68.0); // 100 - 32
        QCOMPARE(state.energy, 80.0); // 100 - 20
        QCOMPARE(state.mood, 76.0);   // 100 - 24
        QCOMPARE(state.fatigue, 16.0);
    }

    void testTwentyFourHoursElapsedDecay() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState initialState;
        initialState.lastCalculatedAt = 100000;
        initialState.hunger = 100.0;
        initialState.energy = 100.0;
        initialState.mood   = 100.0;

        // Advance clock by 24 hours
        testClock->advanceHours(24);

        Pocket::Companion::CompanionState state = simulator.calculateCurrentState(initialState);

        QCOMPARE(state.hunger, 4.0);  // 100 - 96
        QCOMPARE(state.energy, 40.0); // 100 - 60
        QCOMPARE(state.mood, 28.0);   // 100 - 72
        QCOMPARE(state.fatigue, 48.0);
    }

    void testFeedingAction() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState state;
        state.lastCalculatedAt = 100000;
        state.hunger = 50.0;
        state.mood = 50.0;

        testClock->advanceHours(1);

        Pocket::Companion::FeedCompanionCommand cmd;
        cmd.foodAmount = 30.0;
        Pocket::Companion::CompanionState nextState = simulator.executeFeed(state, cmd);

        // After 1 hr decay: hunger=46, mood=47 -> after Feed +30 hunger, +5 mood:
        QCOMPARE(nextState.hunger, 76.0);
        QCOMPARE(nextState.mood, 52.0);
        QCOMPARE(nextState.lastFedAt, static_cast<int64_t>(100000 + 3600));
        QCOMPARE(nextState.bond.xp, static_cast<uint64_t>(10));
    }

    void testRestingAction() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState state;
        state.lastCalculatedAt = 100000;
        state.energy = 20.0;
        state.fatigue = 80.0;

        testClock->advanceHours(2);

        Pocket::Companion::RestCompanionCommand cmd;
        Pocket::Companion::CompanionState nextState = simulator.executeRest(state, cmd);

        QCOMPARE(nextState.energy, 100.0);
        QCOMPARE(nextState.fatigue, 0.0);
        QCOMPARE(nextState.lastRestedAt, static_cast<int64_t>(100000 + 7200));
    }

    void testSaturationBoundsClamping() {
        auto testClock = std::make_shared<Pocket::Companion::TestClock>(100000);
        Pocket::Companion::CompanionSimulator simulator(testClock);

        Pocket::Companion::CompanionState state;
        state.lastCalculatedAt = 100000;
        state.hunger = 100.0;

        // Advance 1000 hours -> would decay below 0, but clamped to 0.0
        testClock->advanceHours(1000);

        Pocket::Companion::CompanionState clampedState = simulator.calculateCurrentState(state);

        QCOMPARE(clampedState.hunger, 0.0);
        QCOMPARE(clampedState.energy, 0.0);
        QCOMPARE(clampedState.mood, 0.0);
        QCOMPARE(clampedState.fatigue, 100.0);
    }
};

QTEST_MAIN(TestCompanionSimulation)
#include "test_companion_simulation.moc"
