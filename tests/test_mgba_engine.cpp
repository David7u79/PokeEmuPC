#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <fstream>
#include "pocket/emulator/MgbaEngine.hpp"
#include "pocket/emulator/PersistentGameSave.hpp"
#include "pocket/emulator/SaveState.hpp"

class TestMgbaEngine : public QObject {
    Q_OBJECT
private slots:
    void testPersistentSaveRoundtrip() {
        QTemporaryFile saveFile;
        QVERIFY(saveFile.open());
        saveFile.close();

        Pocket::Emulator::PersistentGameSave originalSave;
        std::vector<uint8_t> dummySram(512, 0xA5);
        originalSave.setData(dummySram);

        QVERIFY(originalSave.saveToFile(saveFile.fileName().toStdString()));

        Pocket::Emulator::PersistentGameSave loadedSave;
        QVERIFY(loadedSave.loadFromFile(saveFile.fileName().toStdString()));
        QCOMPARE(loadedSave.size(), static_cast<size_t>(512));
        QCOMPARE(loadedSave.data()[0], static_cast<uint8_t>(0xA5));
    }

    void testSaveSafetyRejectsEmptyData() {
        QTemporaryFile saveFile;
        QVERIFY(saveFile.open());
        saveFile.write("VALID_SAVE_HEADER", 17);
        saveFile.close();

        // Attempting to save empty data fails to prevent overwriting valid save
        Pocket::Emulator::PersistentGameSave emptySave;
        QVERIFY(!emptySave.saveToFile(saveFile.fileName().toStdString()));

        // Confirm existing valid save file remains untouched
        Pocket::Emulator::PersistentGameSave verifySave;
        QVERIFY(verifySave.loadFromFile(saveFile.fileName().toStdString()));
        QCOMPARE(verifySave.size(), static_cast<size_t>(17));
    }

    void testNoCoreLifecycle() {
        Pocket::Emulator::MgbaEngine engine;
        QVERIFY(!engine.hasCore());
        QVERIFY(!engine.loadRom("missing.gba"));
        QVERIFY(!engine.isRunning());
    }

    void testMissingCoreReportsError() {
        Pocket::Emulator::MgbaEngine engine("not-a-real-core.dll");
        QVERIFY(!engine.hasCore());
        QVERIFY(!engine.coreError().empty());
    }

    void testEnvironmentHandler() {
        Pocket::Emulator::MgbaEngine engine;
        retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
        QVERIFY(engine.handleEnvironment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format));
        QVERIFY(!engine.handleEnvironment(9999, nullptr));
    }

    // Skipped unless POCKET_MGBA_CORE and POCKET_TEST_ROM point at a real
    // libretro core and ROM: neither can be redistributed with the repo.
    void testRealCoreProducesFrames() {
        const QString corePath = qEnvironmentVariable("POCKET_MGBA_CORE");
        const QString romPath = qEnvironmentVariable("POCKET_TEST_ROM");
        if (corePath.isEmpty() || romPath.isEmpty()) {
            QSKIP("set POCKET_MGBA_CORE and POCKET_TEST_ROM to run this");
        }

        Pocket::Emulator::MgbaEngine engine(corePath.toStdString());
        QVERIFY2(engine.hasCore(), engine.coreError().c_str());
        QVERIFY(engine.loadRom(romPath.toStdString()));

        std::atomic<int> frames{0};
        int width = 0, height = 0;
        engine.setVideoFrameCallback([&](const uint8_t*, int w, int h, size_t) {
            width = w;
            height = h;
            ++frames;
        });

        engine.start();
        QTRY_VERIFY_WITH_TIMEOUT(frames.load() > 30, 5000);
        QCOMPARE(width, 240);
        QCOMPARE(height, 160);

        // GBA runs at 59.7 fps; anything well under that is visibly sluggish.
        const int before = frames.load();
        QElapsedTimer timer;
        timer.start();
        QTest::qWait(2000);
        const double fps = (frames.load() - before) * 1000.0 / timer.elapsed();
        qInfo() << "sustained fps:" << fps;
        QVERIFY2(fps > 55.0, qPrintable(QString("only %1 fps").arg(fps)));

        // A GBA cartridge save must be readable back out of the core.
        QVERIFY(!engine.getPersistentSave().isEmpty());

        engine.stop();
        QVERIFY(!engine.isRunning());
    }

    void testRealCoreSampleRate() {
        const QString corePath = qEnvironmentVariable("POCKET_MGBA_CORE");
        const QString romPath = qEnvironmentVariable("POCKET_TEST_ROM");
        if (corePath.isEmpty() || romPath.isEmpty()) {
            QSKIP("set POCKET_MGBA_CORE and POCKET_TEST_ROM to run this");
        }

        Pocket::Emulator::MgbaEngine engine(corePath.toStdString());
        QVERIFY2(engine.hasCore(), engine.coreError().c_str());
        QVERIFY(engine.loadRom(romPath.toStdString()));

        const double rate = engine.sampleRate();
        qInfo() << "sample rate:" << rate;
        QVERIFY(rate > 0.0);
        QVERIFY(rate != 44100.0);
    }

    // Regression: touching save RAM before retro_load_game crashed inside mGBA.
    void testSaveRamBeforeRomDoesNotCrash() {
        const QString corePath = qEnvironmentVariable("POCKET_MGBA_CORE");
        const QString romPath = qEnvironmentVariable("POCKET_TEST_ROM");
        if (corePath.isEmpty() || romPath.isEmpty()) {
            QSKIP("set POCKET_MGBA_CORE and POCKET_TEST_ROM to run this");
        }

        Pocket::Emulator::MgbaEngine engine(corePath.toStdString());
        QVERIFY2(engine.hasCore(), engine.coreError().c_str());

        Pocket::Emulator::PersistentGameSave save;
        save.setData(std::vector<uint8_t>(131072, 0x5A));

        // No game loaded yet: must be staged, not fatal, and not silently dropped.
        QVERIFY(engine.loadPersistentSave(save));
        QVERIFY(engine.loadRom(romPath.toStdString()));
        QCOMPARE(engine.getPersistentSave().data()[0], static_cast<uint8_t>(0x5A));
    }
};

QTEST_MAIN(TestMgbaEngine)
#include "test_mgba_engine.moc"
