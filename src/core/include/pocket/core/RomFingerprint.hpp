#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Pocket::Core {

struct RomFingerprint {
    std::string crc32;
    std::string md5;
    std::string sha256;
    uint64_t fileSize{0};

    bool isValid() const { return !sha256.empty(); }

    static RomFingerprint calculate(const std::string& romFilePath);
    static RomFingerprint calculateFromBuffer(const std::vector<uint8_t>& buffer);

    static uint32_t calculateCrc32(const uint8_t* data, size_t length);
    static std::string calculateSha256(const uint8_t* data, size_t length);
};

} // namespace Pocket::Core
