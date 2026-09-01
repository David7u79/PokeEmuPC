#pragma once

#include "pocket/save/CreatureSaveParser.hpp"
#include <vector>
#include <cstdint>

namespace Pocket::Save {

class Gen1SaveParser : public CreatureSaveParser {
public:
    Gen1SaveParser() = default;

    SaveParseResult parseSaveFile(const std::string& saveFilePath) override;
    SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) override;

    static uint8_t calculateChecksum(const uint8_t* data, size_t length);
    static std::string decodeGen1Text(const uint8_t* data, size_t maxLen);
    static std::string getGen1SpeciesName(uint8_t speciesId);
};

} // namespace Pocket::Save
