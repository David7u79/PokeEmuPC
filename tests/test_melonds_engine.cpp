#include <QtTest/QtTest>
#include <QApplication>
#include <fstream>
#include "pocket/emulator/MelonDsEngine.hpp"
#include "NdsDisplayWidget.hpp"

class TestMelonDsEngine : public QObject {
    Q_OBJECT

private:
    std::string createSyntheticNdsRom(const std::string& fileName) {
        std::string path = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(path, std::ios::binary);
        std::vector<uint8_t> dummyData(512, 0xFF);
        file.write(reinterpret_cast<const char*>(dummyData.data()), 512);
        file.close();
        return path;
    }

private slots:
    void testMelonDsLifecycle() {
        std::string romPath = createSyntheticNdsRom("test_game.nds");

        Pocket::Emulator::MelonDsEngine engine;
        QCOMPARE(engine.isRunning(), false);
        QCOMPARE(engine.isPaused(), false);

        QVERIFY(engine.loadRom(romPath));
        QCOMPARE(engine.isRunning(), false);

        engine.start();
        QCOMPARE(engine.isRunning(), true);
        QCOMPARE(engine.isPaused(), false);

        engine.pause();
        QCOMPARE(engine.isPaused(), true);

        engine.resume();
        QCOMPARE(engine.isPaused(), false);
        QCOMPARE(engine.isRunning(), true);

        engine.stop();
        QCOMPARE(engine.isRunning(), false);
        QCOMPARE(engine.isPaused(), false);

        QFile::remove(QString::fromStdString(romPath));
        QFile::remove(QString::fromStdString(romPath + ".sav"));
    }

    void testDualScreenFramebuffers() {
        std::string romPath = createSyntheticNdsRom("test_fb.nds");

        Pocket::Emulator::MelonDsEngine engine;
        QVERIFY(engine.loadRom(romPath));

        const uint8_t* top = engine.topFramebuffer();
        const uint8_t* bottom = engine.bottomFramebuffer();

        QVERIFY(top != nullptr);
        QVERIFY(bottom != nullptr);

        // Verify initial synthetic pixels
        QCOMPARE(top[0], static_cast<uint8_t>(0x1A));
        QCOMPARE(bottom[0], static_cast<uint8_t>(0x2E));

        engine.stop();
        QFile::remove(QString::fromStdString(romPath));
        QFile::remove(QString::fromStdString(romPath + ".sav"));
    }

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
        std::string romPath = createSyntheticNdsRom("test_lock.nds");
        auto coordinator = std::make_shared<Pocket::Save::SaveSessionCoordinator>();

        Pocket::Emulator::MelonDsEngine engine(coordinator);
        QVERIFY(engine.loadRom(romPath));

        // Lock should be acquired
        std::string savePath = romPath + ".sav";
        QCOMPARE(coordinator->canMutateSave(savePath), false);

        engine.stop();
        // Lock should be released
        QCOMPARE(coordinator->canMutateSave(savePath), true);

        QFile::remove(QString::fromStdString(romPath));
        QFile::remove(QString::fromStdString(savePath));
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
