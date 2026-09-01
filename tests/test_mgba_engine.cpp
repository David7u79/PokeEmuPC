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

    void testMgbaEngineLifecycleAndInput() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString dummyRomPath = tempDir.path() + "/test_homebrew.gba";

        // Create a minimal 128-byte dummy GBA header file for testing
        std::ofstream file(dummyRomPath.toStdString(), std::ios::binary);
        std::vector<char> dummyHeader(128, 0);
        file.write(dummyHeader.data(), dummyHeader.size());
        file.close();

        Pocket::Emulator::MgbaEngine engine;
        QVERIFY(engine.loadRom(dummyRomPath.toStdString()));

        bool frameReceived = false;
        engine.setVideoFrameCallback([&frameReceived](const uint8_t*, int w, int h, size_t) {
            if (w == 240 && h == 160) {
                frameReceived = true;
            }
        });

        engine.start();
        QVERIFY(engine.isRunning());
        QVERIFY(!engine.isPaused());

        // Test sending button events
        engine.sendButtonEvent(Pocket::Emulator::EmulatorButton::A, true);
        engine.sendButtonEvent(Pocket::Emulator::EmulatorButton::Start, true);

        // Wait for frame execution loop
        QTest::qWait(100);
        QVERIFY(frameReceived);

        engine.pause();
        QVERIFY(engine.isPaused());

        engine.resume();
        QVERIFY(!engine.isPaused());

        // Extract persistent save
        Pocket::Emulator::PersistentGameSave save = engine.getPersistentSave();
        QVERIFY(!save.isEmpty());

        engine.stop();
        QVERIFY(!engine.isRunning());
    }
};

QTEST_MAIN(TestMgbaEngine)
#include "test_mgba_engine.moc"
