#include <QtTest/QtTest>
#include "pocketpartner/core/AppCompanionState.hpp"
#include "pocketpartner/companion/CompanionManager.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include <QTemporaryFile>

class TestCompanionState : public QObject {
    Q_OBJECT
private slots:
    void testLazyElapsedDecay() {
        PocketPartner::Core::AppCompanionState state;
        state.hunger = 100.0;
        state.mood = 100.0;
        state.lastInteractionTs = 100000;

        // Simulate 2 hours elapsed (7200 seconds) without any timer loops
        state.updateElapsed(100000 + 7200);

        QCOMPARE(state.hunger, 92.0); // -4% per hour * 2 hours = 92%
        QCOMPARE(state.mood, 94.0);   // -3% per hour * 2 hours = 94%
        QCOMPARE(state.lastInteractionTs, 100000 + 7200);
    }

    void testInteractionClamping() {
        PocketPartner::Core::AppCompanionState state;
        state.hunger = 95.0;
        state.lastInteractionTs = 1000;

        // Feeding adds +25.0 hunger
        state.hunger += 25.0;
        state.clampValues();

        QCOMPARE(state.hunger, 100.0); // Clamped at 100.0 max
    }
};

QTEST_MAIN(TestCompanionState)
#include "test_companion_state.moc"
