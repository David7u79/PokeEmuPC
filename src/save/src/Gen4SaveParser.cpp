#include "pocket/save/Gen4SaveParser.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Pocket::Save {

static const int kGen4SubstructureOrders[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2},
    {0, 2, 3, 1}, {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2},
    {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1}, {3, 0, 2, 1},
    {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2},
    {2, 3, 0, 1}, {3, 2, 0, 1}, {1, 2, 3, 0}, {1, 3, 2, 0},
    {2, 1, 3, 0}, {3, 1, 2, 0}, {2, 3, 1, 0}, {3, 2, 1, 0}
};

uint16_t Gen4SaveParser::calculateCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void Gen4SaveParser::decryptPokemonData(uint8_t* data136, uint32_t pid, uint16_t checksum) {
    uint32_t seed = checksum;
    uint16_t* words = reinterpret_cast<uint16_t*>(data136 + 0x08);

    for (int i = 0; i < 64; ++i) {
        seed = seed * 0x41C64E6D + 0x60B90885;
        uint16_t key = static_cast<uint16_t>((seed >> 16) & 0xFFFF);
        words[i] ^= key;
    }

    uint8_t rawBlocks[128];
    std::memcpy(rawBlocks, data136 + 0x08, 128);

    int orderIdx = static_cast<int>(((pid & 0x3E) >> 1) % 24);
    const int* order = kGen4SubstructureOrders[orderIdx];

    for (int pos = 0; pos < 4; ++pos) {
        int blockType = order[pos]; // 0=A, 1=B, 2=C, 3=D
        std::memcpy(data136 + 0x08 + (blockType * 32), rawBlocks + (pos * 32), 32);
    }
}

std::string Gen4SaveParser::decodeUtf16Text(const uint8_t* data, size_t charCount) {
    std::string text;
    for (size_t i = 0; i < charCount; ++i) {
        uint16_t ch = static_cast<uint16_t>(data[i * 2]) | (static_cast<uint16_t>(data[i * 2 + 1]) << 8);
        if (ch == 0x0000 || ch == 0xFFFF) break;
        if (ch >= 0x0020 && ch <= 0x007E) {
            text += static_cast<char>(ch);
        } else {
            text += '?';
        }
    }
    return text.empty() ? "POKEMON" : text;
}

std::string Gen4SaveParser::getGen4SpeciesName(uint16_t speciesId) {
    switch (speciesId) {
        case 387: return "Turtwig";
        case 390: return "Chimchar";
        case 393: return "Piplup";
        case 448: return "Lucario";
        case 483: return "Dialga";
        case 484: return "Palkia";
        case 487: return "Giratina";
        case 493: return "Arceus";
        default:  return "Gen IV Pokémon #" + std::to_string(speciesId);
    }
}

Creature Gen4SaveParser::parsePokemonStruct(const uint8_t* rawStruct, const std::string& location) {
    Creature c;
    c.generation = GenerationType::Gen4;
    c.hasFriendship = true;
    c.location = location;

    uint32_t pid = *reinterpret_cast<const uint32_t*>(rawStruct + 0x00);
    uint16_t checksum = *reinterpret_cast<const uint16_t*>(rawStruct + 0x06);

    c.personalityValue = pid;

    uint8_t data136[136];
    std::memcpy(data136, rawStruct, 136);

    decryptPokemonData(data136, pid, checksum);

    const uint8_t* blockA = data136 + 0x08; // Growth
    const uint8_t* blockB = data136 + 0x28; // Attacks
    const uint8_t* blockC = data136 + 0x48; // Nickname
    const uint8_t* blockD = data136 + 0x68; // OT Name

    c.speciesId = *reinterpret_cast<const uint16_t*>(blockA + 0x00);
    c.speciesName = getGen4SpeciesName(c.speciesId);
    c.heldItemId = *reinterpret_cast<const uint16_t*>(blockA + 0x02);
    c.trainer.trainerId = *reinterpret_cast<const uint16_t*>(blockA + 0x04);
    c.trainer.secretId = *reinterpret_cast<const uint16_t*>(blockA + 0x06);
    c.experience = *reinterpret_cast<const uint32_t*>(blockA + 0x08);
    c.friendship.setRawValue(blockA + 0x0C ? blockA[0x0C] : 70);

    c.nickname = decodeUtf16Text(blockC, 11);
    c.trainer.trainerName = decodeUtf16Text(blockD, 7);

    c.evs.hp        = blockB[0x04];
    c.evs.attack    = blockB[0x05];
    c.evs.defense   = blockB[0x06];
    c.evs.speed     = blockB[0x07];
    c.evs.spAttack  = blockB[0x08];
    c.evs.spDefense = blockB[0x09];

    uint32_t iv32 = *reinterpret_cast<const uint32_t*>(blockB + 0x10);
    c.ivs.hp        = (iv32 >> 0)  & 0x1F;
    c.ivs.attack    = (iv32 >> 5)  & 0x1F;
    c.ivs.defense   = (iv32 >> 10) & 0x1F;
    c.ivs.speed     = (iv32 >> 15) & 0x1F;
    c.ivs.spAttack  = (iv32 >> 20) & 0x1F;
    c.ivs.spDefense = (iv32 >> 25) & 0x1F;

    c.nature = static_cast<CreatureNature>(pid % 25);
    c.level = (rawStruct[0x8C] > 0) ? rawStruct[0x8C] : 5;

    return c;
}

SaveParseResult Gen4SaveParser::parseSaveFile(const std::string& saveFilePath) {
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

SaveParseResult Gen4SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) {
    SaveParseResult result;
    if (buffer.size() < 524288) {
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Gen IV save file size must be at least 524,288 bytes.";
        return result;
    }

    uint32_t counterA = *reinterpret_cast<const uint32_t*>(buffer.data() + 0xC0F0);
    uint32_t counterB = *reinterpret_cast<const uint32_t*>(buffer.data() + 0x181F0);

    size_t baseSmall = (counterA >= counterB) ? 0x00000 : 0x0C100;
    size_t baseLarge = (counterA >= counterB) ? 0x18200 : 0x3CF00;

    result.activeSlotIndex = (counterA >= counterB) ? 0 : 1;
    result.saveCounter = std::max(counterA, counterB);
    result.status = SaveParseStatus::Success;

    result.trainerName = decodeUtf16Text(buffer.data() + baseSmall + 0x64, 7);
    result.trainerId = *reinterpret_cast<const uint16_t*>(buffer.data() + baseSmall + 0x74);

    // Party (Small block offset 0x90)
    const uint8_t* partyPtr = buffer.data() + baseSmall + 0x98;
    uint32_t partyCount = std::min<uint32_t>(6, *reinterpret_cast<const uint32_t*>(buffer.data() + baseSmall + 0x94));

    for (uint32_t i = 0; i < partyCount; ++i) {
        const uint8_t* pkmnStruct = partyPtr + (i * 236);
        Creature c = parsePokemonStruct(pkmnStruct, "Party Slot " + std::to_string(i + 1));
        result.party.push_back(c);
    }

    // Boxes (Large block offset 0x04)
    result.boxes.resize(18);
    const uint8_t* boxBase = buffer.data() + baseLarge + 0x04;
    for (int b = 0; b < 18; ++b) {
        for (int s = 0; s < 30; ++s) {
            const uint8_t* pkmnStruct = boxBase + (b * 30 * 136) + (s * 136);
            uint32_t pid = *reinterpret_cast<const uint32_t*>(pkmnStruct);
            if (pid != 0) {
                std::string loc = "Box " + std::to_string(b + 1) + " Slot " + std::to_string(s + 1);
                Creature c = parsePokemonStruct(pkmnStruct, loc);
                result.boxes[b].push_back(c);
            }
        }
    }

    return result;
}

} // namespace Pocket::Save
