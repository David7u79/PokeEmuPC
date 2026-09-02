#include <QtTest/QtTest>
#include <atomic>
#include <thread>

#include "pocket/emulator/AudioRingBuffer.hpp"

using Pocket::Emulator::AudioRingBuffer;

class TestAudioRingBuffer : public QObject {
    Q_OBJECT

private slots:
    void preservesSamplesAndOrder() {
        AudioRingBuffer queue(4);
        const int16_t input[] = {1, 2, 3, 4, 5, 6};
        int16_t output[6]{};
        QCOMPARE(queue.push(input, 3), static_cast<size_t>(3));
        QCOMPARE(queue.pop(output, 3), static_cast<size_t>(3));
        QCOMPARE(std::vector<int16_t>(output, output + 6), std::vector<int16_t>(input, input + 6));
    }

    void overrunIsBounded() {
        AudioRingBuffer queue(2);
        const int16_t input[] = {1, 2, 3, 4, 5, 6};
        QCOMPARE(queue.push(input, 3), static_cast<size_t>(2));
        QCOMPARE(queue.availableFrames(), static_cast<size_t>(2));
        QCOMPARE(queue.stats().droppedOverrun, static_cast<uint64_t>(1));
    }

    void underrunFillsSilence() {
        AudioRingBuffer queue(2);
        int16_t output[] = {7, 7, 7, 7};
        QCOMPARE(queue.pop(output, 2), static_cast<size_t>(0));
        QCOMPARE(std::vector<int16_t>(output, output + 4), std::vector<int16_t>(4, 0));
        QCOMPARE(queue.stats().filledUnderrun, static_cast<uint64_t>(2));
    }

    void wrapsAndClears() {
        AudioRingBuffer queue(3);
        const int16_t first[] = {1, 1, 2, 2, 3, 3};
        const int16_t second[] = {4, 4, 5, 5};
        int16_t discarded[2]{};
        int16_t output[8]{};
        queue.push(first, 3);
        queue.pop(discarded, 1);
        queue.push(second, 2);
        QCOMPARE(queue.availableFrames(), static_cast<size_t>(3));
        queue.pop(output, 4);
        QCOMPARE(std::vector<int16_t>(output, output + 6), std::vector<int16_t>({2, 2, 3, 3, 4, 4}));
        QCOMPARE(output[6], static_cast<int16_t>(0));
        QCOMPARE(output[7], static_cast<int16_t>(0));
        queue.clear();
        QCOMPARE(queue.availableFrames(), static_cast<size_t>(0));
    }

    void singleProducerSingleConsumer() {
        constexpr size_t count = 5000;
        AudioRingBuffer queue(64);
        std::atomic<bool> producerDone{false};
        std::vector<int16_t> received;
        received.reserve(count);
        std::thread producer([&] {
            for (size_t frame = 0; frame < count;) {
                const int16_t sample[] = {static_cast<int16_t>(frame), static_cast<int16_t>(frame)};
                if (queue.push(sample, 1) == 1)
                    ++frame;
                else
                    std::this_thread::yield();
            }
            producerDone = true;
        });
        std::thread consumer([&] {
            while (!producerDone || queue.availableFrames()) {
                int16_t output[2]{};
                if (queue.pop(output, 1) == 1)
                    received.push_back(output[0]);
                else
                    std::this_thread::yield();
            }
        });
        producer.join();
        consumer.join();
        QCOMPARE(received.size(), count);
        for (size_t frame = 0; frame < count; ++frame)
            QCOMPARE(received[frame], static_cast<int16_t>(frame));
    }
};

QTEST_MAIN(TestAudioRingBuffer)
#include "test_audio_ring_buffer.moc"
