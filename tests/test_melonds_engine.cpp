#include <QtTest/QtTest>
#include <QApplication>
#include "pocket/emulator/MelonDsEngine.hpp"
#include "NdsDisplayWidget.hpp"

class TestMelonDsEngine : public QObject {
    Q_OBJECT

private slots:
    void testTouchscreenInputMapping() {
        Pocket::Emulator::MelonDsEngine engine;
        engine.sendTouchInput(128, 96, true);

        int touchX = 0, touchY = 0;
        bool pressed = false;
        engine.getTouchInput(touchX, touchY, pressed);

        QCOMPARE(touchX, 128);
        QCOMPARE(touchY, 96);
        QCOMPARE(pressed, true);

        // Out-of-bounds clamping
        engine.sendTouchInput(300, -10, true);
        engine.getTouchInput(touchX, touchY, pressed);
        QCOMPARE(touchX, 255);
        QCOMPARE(touchY, 0);
    }

    void testSaveSessionLockIntegration() {
        auto coordinator = std::make_shared<Pocket::Save::SaveSessionCoordinator>();
        const std::string savePath = "test_lock.nds.sav";
        QVERIFY(coordinator->acquireEmulatorLock(savePath));
        QCOMPARE(coordinator->canMutateSave(savePath), false);
        QVERIFY(coordinator->releaseEmulatorLock(savePath));
        QCOMPARE(coordinator->canMutateSave(savePath), true);
    }

    void testNdsDisplayLayoutCalculations() {
        Pocket::App::NdsDisplayWidget widget;
        QRect total(0, 0, 500, 600);
        QRect top, bottom;

        // Vertical Layout
        widget.setLayoutMode(Pocket::App::NdsScreenLayout::Vertical);
        widget.calculateScreenRects(total, top, bottom);
        QCOMPARE(top, QRect(0, 0, 500, 300));
        QCOMPARE(bottom, QRect(0, 300, 500, 300));

        // Horizontal Layout
        widget.setLayoutMode(Pocket::App::NdsScreenLayout::Horizontal);
        widget.calculateScreenRects(total, top, bottom);
        QCOMPARE(top, QRect(0, 0, 250, 600));
        QCOMPARE(bottom, QRect(250, 0, 250, 600));

        // Focused Top Layout
        widget.setLayoutMode(Pocket::App::NdsScreenLayout::FocusedTop);
        widget.calculateScreenRects(total, top, bottom);
        QCOMPARE(top, total);
        QCOMPARE(bottom, QRect());
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestMelonDsEngine tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_melonds_engine.moc"
