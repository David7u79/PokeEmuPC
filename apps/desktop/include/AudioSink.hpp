#pragma once

#include "pocket/emulator/AudioRingBuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace Pocket::App {

class AudioSink {
public:
    AudioSink();
    ~AudioSink();
    AudioSink(const AudioSink&) = delete;
    AudioSink& operator=(const AudioSink&) = delete;

    bool open(int sampleRate);
    void close();
    bool isOpen() const;

    void submit(const int16_t* interleavedStereo, size_t frames);

    Pocket::Emulator::AudioRingBuffer::Stats stats() const;
    size_t queuedFrames() const;

private:
#ifdef _WIN32
    static constexpr size_t BufferCount = 8;
    void writeQueuedSamples();

    HWAVEOUT m_waveOut{nullptr};
    WAVEHDR m_waveHeaders[BufferCount]{};
    std::vector<int16_t> m_audioBuffers[BufferCount];
    size_t m_currentBufferIndex{0};
#endif
    mutable std::mutex m_mutex;
    std::unique_ptr<Pocket::Emulator::AudioRingBuffer> m_ringBuffer;
    bool m_isOpen{false};
};

} // namespace Pocket::App
