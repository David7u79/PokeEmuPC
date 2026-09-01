#include <QtTest/QtTest>
#include "CompanionAnimationController.hpp"

class TestFrameratePolicy : public QObject {
    Q_OBJECT
private slots:
    void testFramerateGovernorIntervals() {
        Pocket::CompanionApp::CompanionAnimationController controller;

        controller.setState(Pocket::CompanionApp::AnimationState::Hidden);
        QCOMPARE(controller.targetFps(), 0);

        controller.setState(Pocket::CompanionApp::AnimationState::Static);
        QCOMPARE(controller.targetFps(), 0);

        controller.setState(Pocket::CompanionApp::AnimationState::SlowIdle);
        QCOMPARE(controller.targetFps(), 6);

        controller.triggerInteraction();
        QCOMPARE(controller.targetFps(), 25);
    }
};

QTEST_MAIN(TestFrameratePolicy)
#include "test_framerate_policy.moc"
