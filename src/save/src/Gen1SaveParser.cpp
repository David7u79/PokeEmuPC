#include "pocket/save/Gen1SaveParser.hpp"
#include <fstream>
#include <algorithm>

namespace Pocket::Save {

uint8_t Gen1SaveParser::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum = static_cast<uint8_t>(sum + data[i]);
    }
    return static_cast<uint8_t>(sum ^ 0xFF);
}

std::string Gen1SaveParser::decodeGen1Text(const uint8_t* data, size_t maxLen) {
    std::string text;
    for (size_t i = 0; i < maxLen; ++i) {
        uint8_t b = data[i];
        if (b == 0x50 || b == 0xFF) break; // Terminator
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

std::string Gen1SaveParser::getGen1SpeciesName(uint8_t speciesId) {
    switch (speciesId) {
        case 1:   return "Rhydon";
        case 2:   return "Kangaskhan";
        case 3:   return "Nidoran♂";
        case 4:   return "Clefairy";
        case 5:   return "Spearow";
        case 6:   return "Voltorb";
        case 7:   return "Nidoking";
        case 8:   return "Slowbro";
        case 9:   return "Ivysaur";
        case 10:  return "Exeggutor";
        case 15:  return "Nidoran♀";
        case 16:  return "Nidoqueen";
        case 21:  return "Mewtwo";
        case 84:  return "Pikachu";
        case 112: return "Weedle";
        case 153: return "Bulbasaur";
        case 176: return "Charmander";
        case 177: return "Squirtle";
        default:  return "Gen I Pokémon #" + std::to_string(speciesId);
    }
}

SaveParseResult Gen1SaveParser::parseSaveFile(const std::string& saveFilePath) {
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

SaveParseResult Gen1SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) {
    SaveParseResult result;
    if (buffer.size() < 32768) {
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Gen I save file size must be at least 32,768 bytes.";
        return result;
    }

    uint8_t calculatedChecksum = calculateChecksum(buffer.data() + 0x2598, 0x3523 - 0x2598);
    uint8_t storedChecksum = buffer[0x3523];

    if (calculatedChecksum != storedChecksum) {
        result.status = SaveParseStatus::ChecksumFailed;
        result.errorMessage = "Gen I checksum validation failed.";
        return result;
    }

    result.status = SaveParseStatus::Success;
    result.saveCounter = 1;
    result.activeSlotIndex = 0;
    result.trainerName = decodeGen1Text(buffer.data() + 0x2598, 11);
    result.trainerId = *reinterpret_cast<const uint16_t*>(buffer.data() + 0x2605);

    uint8_t partyCount = std::min<uint8_t>(6, buffer[0x2F2C]);
    const uint8_t* partyPtr = buffer.data() + 0x2F34;
    const uint8_t* otPtr = buffer.data() + 0x3038;
    const uint8_t* nickPtr = buffer.data() + 0x3080;

    for (uint8_t i = 0; i < partyCount; ++i) {
        const uint8_t* pkmn = partyPtr + (i * 44);

        Creature c;
        c.generation = GenerationType::Gen1;
        c.hasFriendship = false;
        c.speciesId = pkmn[0x00];
        c.speciesName = getGen1SpeciesName(static_cast<uint8_t>(c.speciesId));
        c.nickname = decodeGen1Text(nickPtr + (i * 11), 11);

        c.level = pkmn[0x21];
        c.experience = (static_cast<uint32_t>(pkmn[0x0E]) << 16) |
                       (static_cast<uint32_t>(pkmn[0x0F]) << 8)  |
                        static_cast<uint32_t>(pkmn[0x10]);

        c.statExp.hp      = (static_cast<uint16_t>(pkmn[0x11]) << 8) | pkmn[0x12];
        c.statExp.attack  = (static_cast<uint16_t>(pkmn[0x13]) << 8) | pkmn[0x14];
        c.statExp.defense = (static_cast<uint16_t>(pkmn[0x15]) << 8) | pkmn[0x16];
        c.statExp.speed   = (static_cast<uint16_t>(pkmn[0x17]) << 8) | pkmn[0x18];
        c.statExp.special = (static_cast<uint16_t>(pkmn[0x19]) << 8) | pkmn[0x1A];

        uint8_t dv1 = pkmn[0x1B];
        uint8_t dv2 = pkmn[0x1C];

        c.dvs.attack  = (dv1 >> 4) & 0x0F;
        c.dvs.defense = dv1 & 0x0F;
        c.dvs.speed   = (dv2 >> 4) & 0x0F;
        c.dvs.special = dv2 & 0x0F;
        c.dvs.hp      = static_cast<uint8_t>(((c.dvs.attack & 1) << 3) | ((c.dvs.defense & 1) << 2) |
                        ((c.dvs.speed & 1) << 1)  | (c.dvs.special & 1));

        c.trainer.trainerName = decodeGen1Text(otPtr + (i * 11), 11);
        c.trainer.trainerId = (static_cast<uint16_t>(pkmn[0x0C]) << 8) | pkmn[0x0D];

        c.personalityValue = (static_cast<uint32_t>(c.trainer.trainerId) << 16) | (static_cast<uint32_t>(c.speciesId) << 8) | c.level;
        c.location = "Party Slot " + std::to_string(i + 1);

        result.party.push_back(c);
    }

    return result;
}

} // namespace Pocket::Save
