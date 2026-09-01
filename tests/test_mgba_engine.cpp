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
};

QTEST_MAIN(TestMgbaEngine)
#include "test_mgba_engine.moc"
