#pragma once

#include "pocket/save/CreatureSaveParser.hpp"
#include <vector>
#include <cstdint>

namespace Pocket::Save {

class Gen4SaveParser : public CreatureSaveParser {
public:
    Gen4SaveParser() = default;

    SaveParseResult parseSaveFile(const std::string& saveFilePath) override;
    SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) override;

    static uint16_t calculateCrc16(const uint8_t* data, size_t length);
    static void decryptPokemonData(uint8_t* data136, uint32_t pid, uint16_t checksum);
    static std::string decodeUtf16Text(const uint8_t* data, size_t charCount);
    static std::string getGen4SpeciesName(uint16_t speciesId);

    static Creature parsePokemonStruct(const uint8_t* rawStruct, const std::string& location);
};

} // namespace Pocket::Save
