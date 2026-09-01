#include <QtTest/QtTest>
#include "CompanionAnimationController.hpp"
#include "PowerStatusMonitor.hpp"
#include "CompanionStartupManager.hpp"

class TestCompanionWidgetPerformance : public QObject {
    Q_OBJECT

private slots:
    void testAnimationControllerStateTransitions() {
        Pocket::CompanionApp::CompanionAnimationController controller;

        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::Static);
        QCOMPARE(controller.targetFps(), 0);

        controller.setState(Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.targetFps(), 6);

        controller.triggerInteraction(500);
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::Interactive);
        QCOMPARE(controller.targetFps(), 25);

        QTest::qWait(600); // Wait for interaction timer expiration
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.targetFps(), 6);
    }

    void testBatterySaverPolicyEnforcement() {
        Pocket::CompanionApp::CompanionAnimationController controller;
        controller.setState(Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::SlowIdle);

        // Force Battery Saver mode
        controller.updatePowerPolicy(true);
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::Static);
        QCOMPARE(controller.targetFps(), 0);
        QCOMPARE(controller.isBatterySaverEnforced(), true);

        // Restore Normal Power mode
        controller.updatePowerPolicy(false);
        QCOMPARE(controller.currentState(), Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.targetFps(), 6);
    }

    void testPowerStatusQuery() {
        Pocket::CompanionApp::PowerInfo info = Pocket::CompanionApp::PowerStatusMonitor::queryPowerStatus();
        // Verifies Win32 GetSystemPowerStatus does not crash and returns initialized struct
        QVERIFY(info.batteryPercentage >= -1 && info.batteryPercentage <= 100);
    }

    void testStartupManagerToggle() {
        bool originalState = Pocket::CompanionApp::CompanionStartupManager::isAutostartEnabled();

        bool setRes = Pocket::CompanionApp::CompanionStartupManager::setAutostartEnabled(true);
        QVERIFY(setRes);
        QCOMPARE(Pocket::CompanionApp::CompanionStartupManager::isAutostartEnabled(), true);

        // Cleanup: restore original autostart state
        Pocket::CompanionApp::CompanionStartupManager::setAutostartEnabled(originalState);
        QCOMPARE(Pocket::CompanionApp::CompanionStartupManager::isAutostartEnabled(), originalState);
    }
};

QTEST_MAIN(TestCompanionWidgetPerformance)
#include "test_companion_widget_performance.moc"
