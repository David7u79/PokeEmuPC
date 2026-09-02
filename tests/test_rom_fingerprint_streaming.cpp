#include <QtTest/QtTest>
#include <QFile>
#include <QTemporaryDir>
#include "pocket/core/RomFingerprint.hpp"

class TestRomFingerprintStreaming : public QObject {
    Q_OBJECT

private:
    static std::vector<uint8_t> deterministicBytes(size_t size) {
        std::vector<uint8_t> bytes(size);
        uint32_t state = 0x12345678U;
        for (auto& byte : bytes) {
            state = state * 1664525U + 1013904223U;
            byte = static_cast<uint8_t>(state >> 24);
        }
        return bytes;
    }

    static QString writeFile(const QTemporaryDir& directory, const std::vector<uint8_t>& bytes) {
        const QString path = directory.filePath("streaming-test.gba");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<qint64>(bytes.size()));
        return path;
    }

private slots:
    void calculateMatchesBuffer_data() {
        QTest::addColumn<int>("size");
        QTest::newRow("empty") << 0;
        QTest::newRow("one-byte") << 1;
        QTest::newRow("before-block") << 1023;
        QTest::newRow("block") << 65536;
        QTest::newRow("one-megabyte") << (1024 * 1024);
    }

    void calculateMatchesBuffer() {
        QFETCH(int, size);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto bytes = deterministicBytes(static_cast<size_t>(size));
        const QString path = writeFile(directory, bytes);
        QVERIFY(!path.isEmpty());

        const auto streamed = Pocket::Core::RomFingerprint::calculate(path.toStdString());
        const auto buffered = Pocket::Core::RomFingerprint::calculateFromBuffer(bytes);
        QCOMPARE(QString::fromStdString(streamed.crc32), QString::fromStdString(buffered.crc32));
        QCOMPARE(QString::fromStdString(streamed.sha256), QString::fromStdString(buffered.sha256));
        QCOMPARE(QString::fromStdString(streamed.md5), QString::fromStdString(buffered.md5));
        QCOMPARE(streamed.fileSize, buffered.fileSize);
    }

    void crc32MatchesKnownValue() {
        const std::vector<uint8_t> bytes{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        QCOMPARE(Pocket::Core::RomFingerprint::calculateCrc32(bytes.data(), bytes.size()), 0xCBF43926U);
    }

    void progressReportsMonotonicCompletion() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto bytes = deterministicBytes(1024 * 1024);
        const QString path = writeFile(directory, bytes);
        QVERIFY(!path.isEmpty());

        qint64 previous = -1;
        qint64 callbackTotal = -1;
        int calls = 0;
        bool monotonic = true;
        const auto fingerprint = Pocket::Core::RomFingerprint::calculate(path.toStdString(), [&](qint64 done, qint64 total) {
            monotonic = monotonic && done >= previous;
            previous = done;
            callbackTotal = total;
            ++calls;
            return true;
        });
        QVERIFY(fingerprint.isValid());
        QVERIFY(monotonic);
        QVERIFY(calls >= 1);
        QCOMPARE(callbackTotal, static_cast<qint64>(bytes.size()));
        QCOMPARE(previous, callbackTotal);
    }

    void cancellationReturnsEmptyFingerprint() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto bytes = deterministicBytes(1024 * 1024);
        const QString path = writeFile(directory, bytes);
        QVERIFY(!path.isEmpty());

        const auto fingerprint = Pocket::Core::RomFingerprint::calculate(path.toStdString(), [](qint64 done, qint64 total) {
            return done < total / 2;
        });
        QVERIFY(!fingerprint.isValid());
    }

    void missingFileReturnsEmptyFingerprint() {
        const auto fingerprint = Pocket::Core::RomFingerprint::calculate("does-not-exist.gba");
        QVERIFY(!fingerprint.isValid());
    }
};

QTEST_MAIN(TestRomFingerprintStreaming)
#include "test_rom_fingerprint_streaming.moc"
