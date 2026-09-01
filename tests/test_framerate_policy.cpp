#include <QtTest/QtTest>
#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

class TestFrameratePolicy : public QObject {
    Q_OBJECT
private slots:
    void testFramerateGovernorIntervals() {
        PocketPartner::DesktopCompanion::FramerateGovernor governor;

        governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::Hidden);
        QCOMPARE(governor.targetFps(), 0);
        QCOMPARE(governor.intervalMs(), 0);

        governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);
        QCOMPARE(governor.targetFps(), 0);
        QCOMPARE(governor.intervalMs(), 0);

        governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::MinorIdleAnimation);
        QCOMPARE(governor.targetFps(), 8);
        QCOMPARE(governor.intervalMs(), 125); // 1000/8 = 125ms

        governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::InteractiveAnimation);
        QCOMPARE(governor.targetFps(), 25);
        QCOMPARE(governor.intervalMs(), 40); // 1000/25 = 40ms
    }
};

QTEST_MAIN(TestFrameratePolicy)
#include "test_framerate_policy.moc"
