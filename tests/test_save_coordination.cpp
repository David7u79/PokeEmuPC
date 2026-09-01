#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <fstream>
#include "pocket/save/SaveSessionCoordinator.hpp"
#include "pocket/save/FileStabilityVerifier.hpp"
#include "pocket/save/SaveBackupRepository.hpp"

class TestSaveCoordination : public QObject {
    Q_OBJECT
private slots:
    void testConcurrentEmulatorWriteAttempts() {
        Pocket::Save::SaveSessionCoordinator coordinator;
        std::string savePath = "C:/fake_save.sav";

        QCOMPARE(coordinator.getState(savePath), Pocket::Save::SaveSessionState::Available);
        QVERIFY(coordinator.canMutateSave(savePath));

        // Acquire emulator lock
        QVERIFY(coordinator.acquireEmulatorLock(savePath));
        QCOMPARE(coordinator.getState(savePath), Pocket::Save::SaveSessionState::EmulatorActive);

        // RULE: While emulator has loaded a game save, Companion canonical save mutation is FORBIDDEN!
        QVERIFY(!coordinator.canMutateSave(savePath));
        QVERIFY(!coordinator.acquireMutationLock(savePath));

        // Reading is allowed if safe
        QVERIFY(coordinator.canReadSave(savePath));

        // Release emulator lock
        QVERIFY(coordinator.releaseEmulatorLock(savePath));
        QCOMPARE(coordinator.getState(savePath), Pocket::Save::SaveSessionState::Available);

        // Now mutation lock can be acquired
        QVERIFY(coordinator.canMutateSave(savePath));
        QVERIFY(coordinator.acquireMutationLock(savePath));
        QCOMPARE(coordinator.getState(savePath), Pocket::Save::SaveSessionState::MutationActive);

        QVERIFY(coordinator.releaseMutationLock(savePath));
    }

    void testFileWatcherDebounceAndStability() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());

        // File is initially stable
        std::string path = tempFile.fileName().toStdString();
        tempFile.write("DATA_CHUNK_1", 12);
        tempFile.flush();

        QVERIFY(Pocket::Save::FileStabilityVerifier::isFileStable(path, 20, 500));
    }

    void testBackupCreationAndRetentionLimit() {
        QTemporaryDir backupDir;
        QVERIFY(backupDir.isValid());

        QTemporaryFile saveFile;
        QVERIFY(saveFile.open());
        saveFile.write("DUMMY_SRAM_SAVE_DATA", 20);
        saveFile.close();

        Pocket::Save::SaveBackupRepository repo(backupDir.path().toStdString());

        std::string gameId = "game_test_123";

        // Create 12 backups
        for (int i = 0; i < 12; ++i) {
            repo.createBackup(gameId, saveFile.fileName().toStdString(), "REASON_" + std::to_string(i));
            QTest::qWait(10);
        }

        auto backups = repo.getBackupsForGame(gameId);
        // Retention policy trims list to max 10 backups
        QCOMPARE(backups.size(), static_cast<size_t>(10));
    }
};

QTEST_MAIN(TestSaveCoordination)
#include "test_save_coordination.moc"
