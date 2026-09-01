#include "pocket/save/Gen2SaveParser.hpp"
#include <fstream>
#include <algorithm>

namespace Pocket::Save {

uint16_t Gen2SaveParser::calculateChecksum16(const uint8_t* data, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum = static_cast<uint16_t>(sum + data[i]);
    }
    return sum;
}

std::string Gen2SaveParser::decodeGen2Text(const uint8_t* data, size_t maxLen) {
    std::string text;
    for (size_t i = 0; i < maxLen; ++i) {
        uint8_t b = data[i];
        if (b == 0x50 || b == 0xFF) break;
        if (b >= 0x80 && b <= 0x99) {
            text += static_cast<char>('A' + (b - 0x80));
        } else if (b >= 0xA0 && b <= 0xB9) {
            text += static_cast<char>('a' + (b - 0xA0));
        } else if (b >= 0xF6 && b <= 0xFF) {
            text += static_cast<char>('0' + (b - 0xF6));
        } else if (b == 0x7F) {
            text += ' ';
        } else {
            text += '?';
        }
    }
    return text.empty() ? "POKEMON" : text;
}

std::string Gen2SaveParser::getGen2SpeciesName(uint8_t speciesId) {
    switch (speciesId) {
        case 152: return "Chikorita";
        case 155: return "Cyndaquil";
        case 158: return "Totodile";
        case 172: return "Pichu";
        case 175: return "Togepi";
        case 249: return "Lugia";
        case 250: return "Ho-Oh";
        case 251: return "Celebi";
        default:  return "Gen II Pokémon #" + std::to_string(speciesId);
    }
}

SaveParseResult Gen2SaveParser::parseSaveFile(const std::string& saveFilePath) {
    SaveParseResult result;
    std::ifstream file(saveFilePath, std::ios::binary);
    if (!file.is_open()) {
        result.status = SaveParseStatus::NoValidSlotFound;
        result.errorMessage = "Save file not found at " + saveFilePath;
        return result;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return parseSaveBuffer(buffer);
}

SaveParseResult Gen2SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) {
    SaveParseResult result;
    if (buffer.size() < 32768) {
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Gen II save file size must be at least 32,768 bytes.";
        return result;
    }

    // Check Primary Slot (0x2000 - 0x2B82, checksum at 0x2B83-0x2B84)
    uint16_t primaryCalc = calculateChecksum16(buffer.data() + 0x2000, 0x2B83 - 0x2000);
    uint16_t primaryStored = (static_cast<uint16_t>(buffer[0x2B83]) << 8) | buffer[0x2B84];

    size_t baseOffset = 0x2000;
    if (primaryCalc == primaryStored) {
        result.activeSlotIndex = 0;
    } else {
        // Check Secondary Slot (0x0C00 - 0x1782, checksum at 0x1783-0x1784)
        uint16_t secCalc = calculateChecksum16(buffer.data() + 0x0C00, 0x1783 - 0x0C00);
        uint16_t secStored = (static_cast<uint16_t>(buffer[0x1783]) << 8) | buffer[0x1784];

        if (secCalc == secStored) {
            result.activeSlotIndex = 1;
            baseOffset = 0x0C00;
        } else {
            result.status = SaveParseStatus::ChecksumFailed;
            result.errorMessage = "Gen II primary and secondary checksum validation failed.";
            return result;
        }
    }

    result.status = SaveParseStatus::Success;
    result.saveCounter = 1;
    result.trainerName = decodeGen2Text(buffer.data() + baseOffset, 11);
    result.trainerId = *reinterpret_cast<const uint16_t*>(buffer.data() + baseOffset + 0xA9);

    size_t partyBase = (result.activeSlotIndex == 0) ? 0x2865 : 0x1065;
    uint8_t partyCount = std::min<uint8_t>(6, buffer[partyBase]);
    const uint8_t* partyPtr = buffer.data() + partyBase + 8; // 0x286D
    const uint8_t* otPtr = buffer.data() + partyBase + 296;  // 0x298D
    const uint8_t* nickPtr = buffer.data() + partyBase + 368; // 0x29D5

    for (uint8_t i = 0; i < partyCount; ++i) {
        const uint8_t* pkmn = partyPtr + (i * 48);

        Creature c;
        c.generation = GenerationType::Gen2;
        c.hasFriendship = true;
        c.friendship.setRawValue(pkmn[0x1B]);

        c.speciesId = pkmn[0x00];
        c.speciesName = getGen2SpeciesName(static_cast<uint8_t>(c.speciesId));
        c.nickname = decodeGen2Text(nickPtr + (i * 11), 11);

        c.level = pkmn[0x1F];
        c.experience = (static_cast<uint32_t>(pkmn[0x08]) << 16) |
                       (static_cast<uint32_t>(pkmn[0x09]) << 8)  |
                        static_cast<uint32_t>(pkmn[0x0A]);

        c.statExp.hp      = (static_cast<uint16_t>(pkmn[0x0B]) << 8) | pkmn[0x0C];
        c.statExp.attack  = (static_cast<uint16_t>(pkmn[0x0D]) << 8) | pkmn[0x0E];
        c.statExp.defense = (static_cast<uint16_t>(pkmn[0x0F]) << 8) | pkmn[0x10];
        c.statExp.speed   = (static_cast<uint16_t>(pkmn[0x11]) << 8) | pkmn[0x12];
        c.statExp.special = (static_cast<uint16_t>(pkmn[0x13]) << 8) | pkmn[0x14];

        uint8_t dv1 = pkmn[0x15];
        uint8_t dv2 = pkmn[0x16];

        c.dvs.attack  = (dv1 >> 4) & 0x0F;
        c.dvs.defense = dv1 & 0x0F;
        c.dvs.speed   = (dv2 >> 4) & 0x0F;
        c.dvs.special = dv2 & 0x0F;
        c.dvs.hp      = static_cast<uint8_t>(((c.dvs.attack & 1) << 3) | ((c.dvs.defense & 1) << 2) |
                        ((c.dvs.speed & 1) << 1)  | (c.dvs.special & 1));

        c.trainer.trainerName = decodeGen2Text(otPtr + (i * 11), 11);
        c.trainer.trainerId = (static_cast<uint16_t>(pkmn[0x06]) << 8) | pkmn[0x07];

        c.personalityValue = (static_cast<uint32_t>(c.trainer.trainerId) << 16) | (static_cast<uint32_t>(c.speciesId) << 8) | c.level;
        c.location = "Party Slot " + std::to_string(i + 1);

        result.party.push_back(c);
    }

    return result;
}

} // namespace Pocket::Save
