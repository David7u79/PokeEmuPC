#pragma once

#include <vector>
#include <string>
#include "pocket/save/CreatureSaveParser.hpp"

namespace Pocket::Save {

class Gen3SaveParser : public CreatureSaveParser {
public:
    Gen3SaveParser() = default;

    SaveParseResult parseSaveFile(const std::string& filePath) override;
    SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) override;

    static uint16_t calculateSectionChecksum(const uint8_t* sectionData);
    static std::string decodeGen3String(const uint8_t* data, size_t length);
    static void decryptPokemonData(uint8_t* data80, uint32_t pid, uint32_t otId);
    static Creature parsePokemonStruct(const uint8_t* raw100, const std::string& location);

private:
    struct SlotInfo {
        bool isValid{false};
        int slotIndex{0}; // 0 = Slot A, 1 = Slot B
        uint32_t saveCounter{0};
        const uint8_t* sectionPointers[14]{};
    };

    SlotInfo validateAndMapSlot(const uint8_t* slotData, int slotIdx);
};

} // namespace Pocket::Save
