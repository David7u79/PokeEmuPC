#pragma once

#include "pocket/save/CreatureSaveParser.hpp"
#include <vector>
#include <cstdint>

namespace Pocket::Save {

class Gen2SaveParser : public CreatureSaveParser {
public:
    Gen2SaveParser() = default;

    SaveParseResult parseSaveFile(const std::string& saveFilePath) override;
    SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) override;

    static uint16_t calculateChecksum16(const uint8_t* data, size_t length);
    static std::string decodeGen2Text(const uint8_t* data, size_t maxLen);
    static std::string getGen2SpeciesName(uint8_t speciesId);
};

} // namespace Pocket::Save
