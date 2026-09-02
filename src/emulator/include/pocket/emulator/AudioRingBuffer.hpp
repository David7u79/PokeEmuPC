#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace Pocket::Emulator {
class AudioRingBuffer {
public:
    struct Stats {
        uint64_t pushed{0};
        uint64_t popped{0};
        uint64_t droppedOverrun{0};
        uint64_t filledUnderrun{0};
    };

    explicit AudioRingBuffer(size_t capacityFrames);
    size_t push(const int16_t* interleavedStereo, size_t frames);
    size_t pop(int16_t* out, size_t frames);
    size_t capacityFrames() const;
    size_t availableFrames() const;
    void clear();
    Stats stats() const;
    void resetStats();

private:
    mutable std::mutex m_mutex;
    std::vector<int16_t> m_samples;
    size_t m_read{0};
    size_t m_write{0};
    size_t m_available{0};
    Stats m_stats;
};
} // namespace Pocket::Emulator
