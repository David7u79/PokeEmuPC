#include "pocket/save/Gen5SaveParser.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Pocket::Save {

static const int kGen5SubstructureOrders[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2},
    {0, 2, 3, 1}, {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2},
    {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1}, {3, 0, 2, 1},
    {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2},
    {2, 3, 0, 1}, {3, 2, 0, 1}, {1, 2, 3, 0}, {1, 3, 2, 0},
    {2, 1, 3, 0}, {3, 1, 2, 0}, {2, 3, 1, 0}, {3, 2, 1, 0}
};

uint16_t Gen5SaveParser::calculateCrc16(const uint8_t* data, size_t length) {
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

void Gen5SaveParser::decryptPokemonData(uint8_t* data136, uint32_t pid, uint16_t checksum) {
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
    const int* order = kGen5SubstructureOrders[orderIdx];

    for (int pos = 0; pos < 4; ++pos) {
        int blockType = order[pos]; // 0=A, 1=B, 2=C, 3=D
        std::memcpy(data136 + 0x08 + (blockType * 32), rawBlocks + (pos * 32), 32);
    }
}

std::string Gen5SaveParser::decodeUtf16Text(const uint8_t* data, size_t charCount) {
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

std::string Gen5SaveParser::getGen5SpeciesName(uint16_t speciesId) {
    switch (speciesId) {
        case 494: return "Victini";
        case 495: return "Snivy";
        case 498: return "Tepig";
        case 501: return "Oshawott";
        case 570: return "Zorua";
        case 571: return "Zoroark";
        case 643: return "Reshiram";
        case 644: return "Zekrom";
        case 646: return "Kyurem";
        case 647: return "Keldeo";
        default:  return "Gen V Pokémon #" + std::to_string(speciesId);
    }
}

Creature Gen5SaveParser::parsePokemonStruct(const uint8_t* rawStruct, const std::string& location) {
    Creature c;
    c.generation = GenerationType::Gen5;
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
    c.speciesName = getGen5SpeciesName(c.speciesId);
    c.heldItemId = *reinterpret_cast<const uint16_t*>(blockA + 0x02);
    c.trainer.trainerId = *reinterpret_cast<const uint16_t*>(blockA + 0x04);
    c.trainer.secretId = *reinterpret_cast<const uint16_t*>(blockA + 0x06);
    c.experience = *reinterpret_cast<const uint32_t*>(blockA + 0x08);
    c.friendship.setRawValue(blockA[0x0C] ? blockA[0x0C] : 70);

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

SaveParseResult Gen5SaveParser::parseSaveFile(const std::string& saveFilePath) {
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

SaveParseResult Gen5SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) {
    SaveParseResult result;
    if (buffer.size() < 524288) {
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Gen V save file size must be at least 524,288 bytes.";
        return result;
    }

    result.activeSlotIndex = 0;
    result.saveCounter = 1;
    result.status = SaveParseStatus::Success;

    result.trainerName = decodeUtf16Text(buffer.data() + 0x19404, 7);
    result.trainerId = *reinterpret_cast<const uint16_t*>(buffer.data() + 0x19414);

    // Party (Offset 0x18E08)
    const uint8_t* partyPtr = buffer.data() + 0x18E08;
    uint32_t partyCount = std::min<uint32_t>(6, *reinterpret_cast<const uint32_t*>(buffer.data() + 0x18E04));

    for (uint32_t i = 0; i < partyCount; ++i) {
        const uint8_t* pkmnStruct = partyPtr + (i * 220);
        Creature c = parsePokemonStruct(pkmnStruct, "Party Slot " + std::to_string(i + 1));
        result.party.push_back(c);
    }

    // Boxes (Offset 0x26000)
    result.boxes.resize(24);
    const uint8_t* boxBase = buffer.data() + 0x26000;
    for (int b = 0; b < 24; ++b) {
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
