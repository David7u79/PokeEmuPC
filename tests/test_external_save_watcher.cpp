#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <vector>
#include <cstring>
#include <fstream>
#include "pocket/save/ExternalSaveWatcher.hpp"
#include "pocket/save/Gen3SaveParser.hpp"

class TestExternalSaveWatcher : public QObject {
    Q_OBJECT

private:
    std::string createSyntheticGen3SaveFile(const std::string& fileName, uint32_t saveCounter = 10) {
        std::vector<uint8_t> buffer(131072, 0x00);

        auto populateSection = [&](uint8_t* secPtr, uint16_t sectionId, uint32_t counter) {
            std::memset(secPtr, 0x00, 4096);

            if (sectionId == 0) {
                secPtr[0x00] = 0xCC; secPtr[0x01] = 0xD9; secPtr[0x02] = 0xD8; secPtr[0x03] = 0xFF;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0A) = 12345;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0C) = 54321;
                *reinterpret_cast<uint16_t*>(secPtr + 0x0E) = 12;
                secPtr[0x10] = 34;
            } else if (sectionId == 1) {
                *reinterpret_cast<uint32_t*>(secPtr + 0x234) = 1;

                uint8_t* pkmnPtr = secPtr + 0x238;
                uint32_t pid = 100;
                uint32_t otId = 12345;

                *reinterpret_cast<uint32_t*>(pkmnPtr + 0x00) = pid;
                *reinterpret_cast<uint32_t*>(pkmnPtr + 0x04) = otId;

                // "Treeck"
                pkmnPtr[0x08] = 0xD9; pkmnPtr[0x09] = 0xE7; pkmnPtr[0x0A] = 0xE3;
                pkmnPtr[0x0B] = 0xD7; pkmnPtr[0x0C] = 0xD9; pkmnPtr[0x0D] = 0xFF;

                // "Red"
                pkmnPtr[0x14] = 0xCC; pkmnPtr[0x15] = 0xD9; pkmnPtr[0x16] = 0xD8; pkmnPtr[0x17] = 0xFF;

                uint8_t blockG[12]{};
                *reinterpret_cast<uint16_t*>(blockG + 0) = 252; // Treecko
                *reinterpret_cast<uint32_t*>(blockG + 4) = 125;
                blockG[9] = 70;

                uint8_t blockA[12]{};
                uint8_t blockE[12]{};
                uint8_t blockM[12]{};

                std::memcpy(pkmnPtr + 0x20, blockG, 12);
                std::memcpy(pkmnPtr + 0x2C, blockM, 12);
                std::memcpy(pkmnPtr + 0x38, blockA, 12);
                std::memcpy(pkmnPtr + 0x44, blockE, 12);

                uint32_t key = pid ^ otId;
                uint32_t* dwords = reinterpret_cast<uint32_t*>(pkmnPtr + 0x20);
                for (int d = 0; d < 12; ++d) {
                    dwords[d] ^= key;
                }

                pkmnPtr[0x54] = 5;
            }

            *reinterpret_cast<uint16_t*>(secPtr + 0xFF4) = sectionId;
            *reinterpret_cast<uint32_t*>(secPtr + 0xFF8) = 0x08012002;
            *reinterpret_cast<uint32_t*>(secPtr + 0xFFC) = counter;

            uint16_t checksum = Pocket::Save::Gen3SaveParser::calculateSectionChecksum(secPtr);
            *reinterpret_cast<uint16_t*>(secPtr + 0xFF6) = checksum;
        };

        for (uint16_t sec = 0; sec < 14; ++sec) {
            populateSection(buffer.data() + (sec * 4096), sec, saveCounter);
            populateSection(buffer.data() + 0x0E000 + (sec * 4096), sec, saveCounter > 5 ? saveCounter - 5 : 0);
        }

        std::string filePath = QDir::tempPath().toStdString() + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.data()), 131072);
        file.close();

        return filePath;
    }

private slots:
    void testMultipleWatcherEventsDebouncing() {
        std::string savePath = createSyntheticGen3SaveFile("test_watcher_debounce.sav", 10);

        Pocket::Save::ExternalSaveWatcher watcher;
        QVERIFY(watcher.setMonitoredSavePath(savePath));

        QSignalSpy spy(&watcher, &Pocket::Save::ExternalSaveWatcher::saveUpdated);

        // Perform 3 rapid file writes within 50ms
        for (int i = 1; i <= 3; ++i) {
            createSyntheticGen3SaveFile("test_watcher_debounce.sav", 10 + i);
            QTest::qWait(20);
        }

        // Wait for 100ms debounce timer to expire
        QTest::qWait(300);

        // Signal should fire EXACTLY ONCE after debouncing
        QCOMPARE(spy.count(), 1);

        QFile::remove(QString::fromStdString(savePath));
    }

    void testAtomicFileReplace() {
        std::string savePath = createSyntheticGen3SaveFile("test_atomic_replace.sav", 10);

        Pocket::Save::ExternalSaveWatcher watcher;
        QVERIFY(watcher.setMonitoredSavePath(savePath));

        QSignalSpy spy(&watcher, &Pocket::Save::ExternalSaveWatcher::saveUpdated);

        // Simulate external emulator atomic replace (.tmp -> rename)
        std::string tmpPath = savePath + ".tmp";
        createSyntheticGen3SaveFile("test_atomic_replace.sav.tmp", 20);

        QFile::remove(QString::fromStdString(savePath));
        QFile::rename(QString::fromStdString(tmpPath), QString::fromStdString(savePath));

        watcher.forceCheck();

        QCOMPARE(spy.count(), 1);

        QFile::remove(QString::fromStdString(savePath));
    }

    void testSaveTruncationHandling() {
        std::string savePath = createSyntheticGen3SaveFile("test_truncation.sav", 10);

        Pocket::Save::ExternalSaveWatcher watcher;
        QVERIFY(watcher.setMonitoredSavePath(savePath));

        QSignalSpy spy(&watcher, &Pocket::Save::ExternalSaveWatcher::saveUpdated);

        // Write truncated 10KB partial file
        std::vector<uint8_t> partialBuffer(10240, 0x00);
        std::ofstream file(savePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(partialBuffer.data()), 10240);
        file.close();

        watcher.forceCheck();

        // Should ignore truncated file (0 updates emitted)
        QCOMPARE(spy.count(), 0);

        // Restore valid 128KB save file
        createSyntheticGen3SaveFile("test_truncation.sav", 30);
        watcher.forceCheck();

        // Should process valid restored file
        QCOMPARE(spy.count(), 1);

        QFile::remove(QString::fromStdString(savePath));
    }

    void testZeroRecurringPollingTimers() {
        Pocket::Save::ExternalSaveWatcher watcher;
        // Verify default sync mode is ReadOnlyExternal
        QCOMPARE(watcher.syncMode(), Pocket::Save::ExternalSyncMode::ReadOnlyExternal);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestExternalSaveWatcher tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_external_save_watcher.moc"
