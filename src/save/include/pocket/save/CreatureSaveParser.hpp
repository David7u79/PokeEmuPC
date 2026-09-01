#pragma once

#include <string>
#include <vector>
#include "pocket/save/Creature.hpp"

namespace Pocket::Save {

enum class SaveParseStatus {
    Success,
    InvalidFileLength,
    ChecksumFailed,
    NoValidSlotFound
};

struct SaveParseResult {
    SaveParseStatus status{SaveParseStatus::NoValidSlotFound};
    std::string errorMessage;

    int activeSlotIndex{-1}; // 0 = Slot A, 1 = Slot B
    uint32_t saveCounter{0};

    std::string trainerName;
    uint16_t trainerId{0};
    uint16_t secretId{0};
    uint32_t money{0};
    uint16_t playTimeHours{0};
    uint8_t playTimeMinutes{0};

    std::vector<Creature> party;
    std::vector<std::vector<Creature>> boxes; // 14 Boxes x 30 Slots
};

class CreatureSaveParser {
public:
    virtual ~CreatureSaveParser() = default;
    virtual SaveParseResult parseSaveFile(const std::string& filePath) = 0;
    virtual SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) = 0;
};

} // namespace Pocket::Save
