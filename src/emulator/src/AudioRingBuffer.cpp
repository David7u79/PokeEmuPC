#include "pocket/emulator/AudioRingBuffer.hpp"
#include <algorithm>
#include <cstring>

namespace Pocket::Emulator {
AudioRingBuffer::AudioRingBuffer(size_t capacityFrames) : m_samples(capacityFrames * 2) {}

size_t AudioRingBuffer::push(const int16_t* samples, size_t frames) {
    if (!samples || !frames)
        return 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    const size_t capacity = capacityFrames();
    const size_t accepted = std::min(frames, capacity - m_available);
    for (size_t frame = 0; frame < accepted; ++frame) {
        std::memcpy(&m_samples[m_write * 2], samples + frame * 2, 2 * sizeof(int16_t));
        m_write = capacity ? (m_write + 1) % capacity : 0;
    }
    m_available += accepted;
    m_stats.pushed += accepted;
    m_stats.droppedOverrun += frames - accepted;
    return accepted;
}

size_t AudioRingBuffer::pop(int16_t* out, size_t frames) {
    if (!out || !frames)
        return 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    const size_t capacity = capacityFrames();
    const size_t read = std::min(frames, m_available);
    for (size_t frame = 0; frame < read; ++frame) {
        std::memcpy(out + frame * 2, &m_samples[m_read * 2], 2 * sizeof(int16_t));
        m_read = capacity ? (m_read + 1) % capacity : 0;
    }
    std::fill(out + read * 2, out + frames * 2, 0);
    m_available -= read;
    m_stats.popped += read;
    m_stats.filledUnderrun += frames - read;
    return read;
}

size_t AudioRingBuffer::capacityFrames() const {
    return m_samples.size() / 2;
}

size_t AudioRingBuffer::availableFrames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_available;
}

void AudioRingBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_read = 0;
    m_write = 0;
    m_available = 0;
}

AudioRingBuffer::Stats AudioRingBuffer::stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void AudioRingBuffer::resetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = {};
}
} // namespace Pocket::Emulator
