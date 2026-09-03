#include "AudioSink.hpp"

#include <algorithm>
#include <cstring>

namespace Pocket::App {

void applyVolume(int16_t* samples, size_t count, float volume) {
    if (!samples || volume == 1.0f)
        return;
    volume = std::clamp(volume, 0.0f, 1.0f);
    for (size_t i = 0; i < count; ++i) {
        const int value = static_cast<int>(samples[i] * volume);
        samples[i] = static_cast<int16_t>(std::clamp(value, -32768, 32767));
    }
}

AudioSink::AudioSink() = default;

AudioSink::~AudioSink() {
    close();
}

bool AudioSink::open(int sampleRate) {
    close();
    if (sampleRate <= 0)
        return false;

#ifdef _WIN32
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = static_cast<DWORD>(sampleRate);
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (waveOutOpen(&m_waveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        return false;

    const size_t capacityFrames = std::max<size_t>(1, (static_cast<size_t>(sampleRate) + 9) / 10);
    m_ringBuffer = std::make_unique<Pocket::Emulator::AudioRingBuffer>(capacityFrames);
    m_currentBufferIndex = 0;
    for (size_t i = 0; i < BufferCount; ++i) {
        m_audioBuffers[i].resize(capacityFrames * 2, 0);
        std::memset(&m_waveHeaders[i], 0, sizeof(WAVEHDR));
        m_waveHeaders[i].lpData = reinterpret_cast<LPSTR>(m_audioBuffers[i].data());
        m_waveHeaders[i].dwBufferLength = static_cast<DWORD>(m_audioBuffers[i].size() * sizeof(int16_t));
        waveOutPrepareHeader(m_waveOut, &m_waveHeaders[i], sizeof(WAVEHDR));
    }
    m_isOpen = true;
    return true;
#else
    static_cast<void>(sampleRate);
    return false;
#endif
}

void AudioSink::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
#ifdef _WIN32
    if (m_waveOut) {
        waveOutReset(m_waveOut);
        for (size_t i = 0; i < BufferCount; ++i) {
            if (m_waveHeaders[i].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(m_waveOut, &m_waveHeaders[i], sizeof(WAVEHDR));
            std::memset(&m_waveHeaders[i], 0, sizeof(WAVEHDR));
            m_audioBuffers[i].clear();
        }
        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }
    m_currentBufferIndex = 0;
#endif
    m_ringBuffer.reset();
    m_isOpen = false;
}

bool AudioSink::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isOpen;
}

void AudioSink::submit(const int16_t* interleavedStereo, size_t frames) {
    if (!interleavedStereo || frames == 0)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isOpen || !m_ringBuffer)
        return;

    if (m_volume == 1.0f) {
        m_ringBuffer->push(interleavedStereo, frames);
    } else {
        std::vector<int16_t> scaled(interleavedStereo, interleavedStereo + frames * 2);
        applyVolume(scaled.data(), scaled.size(), m_volume);
        m_ringBuffer->push(scaled.data(), frames);
    }
#ifdef _WIN32
    writeQueuedSamples();
#endif
}

void AudioSink::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_volume = std::clamp(volume, 0.0f, 1.0f);
}

float AudioSink::volume() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_volume;
}

Pocket::Emulator::AudioRingBuffer::Stats AudioSink::stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ringBuffer ? m_ringBuffer->stats() : Pocket::Emulator::AudioRingBuffer::Stats{};
}

size_t AudioSink::queuedFrames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ringBuffer ? m_ringBuffer->availableFrames() : 0;
}

#ifdef _WIN32
void AudioSink::writeQueuedSamples() {
    if (!m_waveOut || !m_ringBuffer)
        return;

    WAVEHDR& header = m_waveHeaders[m_currentBufferIndex];
    if ((header.dwFlags & WHDR_INQUEUE) && !(header.dwFlags & WHDR_DONE))
        return;

    auto& buffer = m_audioBuffers[m_currentBufferIndex];
    const size_t frames = (std::min)(m_ringBuffer->availableFrames(), buffer.size() / 2);
    if (frames == 0)
        return;

    const size_t sampleCount = frames * 2;
    if (buffer.size() < sampleCount) {
        // Resizing moves the buffer. Re-prepare the header so the driver never
        // retains a pointer to freed memory.
        waveOutUnprepareHeader(m_waveOut, &header, sizeof(WAVEHDR));
        buffer.resize(sampleCount);
        header.lpData = reinterpret_cast<LPSTR>(buffer.data());
        header.dwBufferLength = static_cast<DWORD>(sampleCount * sizeof(int16_t));
        header.dwFlags = 0;
        waveOutPrepareHeader(m_waveOut, &header, sizeof(WAVEHDR));
    }

    m_ringBuffer->pop(buffer.data(), frames);
    header.dwBufferLength = static_cast<DWORD>(sampleCount * sizeof(int16_t));
    waveOutWrite(m_waveOut, &header, sizeof(WAVEHDR));
    m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
}
#endif

} // namespace Pocket::App
