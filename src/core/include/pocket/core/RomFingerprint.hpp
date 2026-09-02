#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <QtGlobal>

namespace Pocket::Core {

struct RomFingerprint {
    using ProgressCallback = std::function<bool(qint64 done, qint64 total)>;

    std::string crc32;
    std::string md5;
    std::string sha256;
    uint64_t fileSize{0};

    bool isValid() const { return !sha256.empty(); }

    static RomFingerprint calculate(const std::string& romFilePath);
    static RomFingerprint calculate(const std::string& romFilePath, const ProgressCallback& progress);
    static RomFingerprint calculateFromBuffer(const std::vector<uint8_t>& buffer);

    static uint32_t calculateCrc32(const uint8_t* data, size_t length);
    static std::string calculateSha256(const uint8_t* data, size_t length);
};

} // namespace Pocket::Core
