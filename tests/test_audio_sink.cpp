#include "AudioSink.hpp"

#include <QTest>

#include <atomic>
#include <thread>
#include <vector>

class AudioSinkTest : public QObject {
    Q_OBJECT

private slots:
    void opensAtCoreSampleRates() {
        Pocket::App::AudioSink sink;
        if (!sink.open(32728))
            QSKIP("No audio output device is available on this test machine.");
        QVERIFY(sink.isOpen());
        QVERIFY(sink.open(65536));
        QVERIFY(sink.isOpen());
    }

    void submitRecordsFrames() {
        Pocket::App::AudioSink sink;
        if (!sink.open(32728))
            QSKIP("No audio output device is available on this test machine.");
        std::vector<int16_t> samples(200 * 2);
        sink.submit(samples.data(), 200);
        QCOMPARE(sink.stats().pushed, uint64_t{200});
    }

    void overrunIsReported() {
        Pocket::App::AudioSink sink;
        if (!sink.open(32728))
            QSKIP("No audio output device is available on this test machine.");
        std::vector<int16_t> samples(32728 * 4 * 2);
        sink.submit(samples.data(), 32728 * 4);
        QVERIFY(sink.stats().droppedOverrun > 0);
        QVERIFY(sink.queuedFrames() <= 3273);
    }

    void ignoresEmptyAndClosedSubmissions() {
        Pocket::App::AudioSink sink;
        if (!sink.open(32728))
            QSKIP("No audio output device is available on this test machine.");
        sink.submit(nullptr, 0);
        sink.submit(nullptr, 1);
        QCOMPARE(sink.stats().pushed, uint64_t{0});
        sink.close();
        sink.submit(nullptr, 0);
        QVERIFY(!sink.isOpen());
    }

    void acceptsConcurrentSubmission() {
        Pocket::App::AudioSink sink;
        if (!sink.open(32728))
            QSKIP("No audio output device is available on this test machine.");
        std::vector<int16_t> samples(128 * 2);
        std::atomic<bool> start{false};
        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }
            for (int i = 0; i < 100; ++i)
                sink.submit(samples.data(), 128);
        });
        start.store(true, std::memory_order_release);
        producer.join();
        QVERIFY(sink.stats().pushed > 0);
    }
};

QTEST_MAIN(AudioSinkTest)
#include "test_audio_sink.moc"
