#include "pocket/save/Gen3SaveParser.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Pocket::Save {

static const char* kGen3SpeciesNames[] = {
    "None", "Bulbasaur", "Ivysaur", "Venusaur", "Charmander", "Charmeleon", "Charizard",
    "Squirtle", "Wartortle", "Blastoise", "Caterpie", "Metapod", "Butterfree",
    "Weedle", "Kakuna", "Beedrill", "Pidgey", "Pidgeotto", "Pidgeot", "Rattata", "Raticate",
    "Spearow", "Fearow", "Ekans", "Arbok", "Pikachu", "Raichu", "Sandshrew", "Sandslash"
};

uint16_t Gen3SaveParser::calculateSectionChecksum(const uint8_t* sectionData) {
    uint32_t sum = 0;
    for (size_t i = 0; i < 0xFF4; i += 4) {
        uint32_t val = static_cast<uint32_t>(sectionData[i]) |
                      (static_cast<uint32_t>(sectionData[i + 1]) << 8) |
                      (static_cast<uint32_t>(sectionData[i + 2]) << 16) |
                      (static_cast<uint32_t>(sectionData[i + 3]) << 24);
        sum += val;
    }
    uint16_t upper = static_cast<uint16_t>(sum >> 16);
    uint16_t lower = static_cast<uint16_t>(sum & 0xFFFF);
    return static_cast<uint16_t>(upper + lower);
}

std::string Gen3SaveParser::decodeGen3String(const uint8_t* data, size_t length) {
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        if (byte == 0xFF) break; // String terminator

        if (byte >= 0xBB && byte <= 0xD4) {
            result += static_cast<char>('A' + (byte - 0xBB));
        } else if (byte >= 0xD5 && byte <= 0xEE) {
            result += static_cast<char>('a' + (byte - 0xD5));
        } else if (byte >= 0xA1 && byte <= 0xAA) {
            result += static_cast<char>('0' + (byte - 0xA1));
        } else if (byte == 0x00) {
            result += ' ';
        } else {
            result += '?';
        }
    }
    return result.empty() ? "Partner" : result;
}

static const int kSubstructureOrders[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}
};

void Gen3SaveParser::decryptPokemonData(uint8_t* data80, uint32_t pid, uint32_t otId) {
    uint32_t key = pid ^ otId;
    uint32_t* dwords = reinterpret_cast<uint32_t*>(data80 + 0x20);

    // Decrypt 12 dwords (48 bytes)
    for (int i = 0; i < 12; ++i) {
        dwords[i] ^= key;
    }

    // Unshuffle 4 blocks into canonical order: Block 0=G, Block 1=A, Block 2=E, Block 3=M
    uint8_t rawBlocks[48];
    std::memcpy(rawBlocks, data80 + 0x20, 48);

    int orderIdx = static_cast<int>(pid % 24);
    const int* order = kSubstructureOrders[orderIdx];

    for (int pos = 0; pos < 4; ++pos) {
        int blockType = order[pos]; // 0=G, 1=A, 2=E, 3=M
        std::memcpy(data80 + 0x20 + (blockType * 12), rawBlocks + (pos * 12), 12);
    }
}

Creature Gen3SaveParser::parsePokemonStruct(const uint8_t* raw100, const std::string& location) {
    Creature c;
    c.generation = 3;
    c.location = location;

    uint32_t pid = *reinterpret_cast<const uint32_t*>(raw100 + 0x00);
    uint32_t otId = *reinterpret_cast<const uint32_t*>(raw100 + 0x04);
    c.personalityValue = pid;

    c.nickname = decodeGen3String(raw100 + 0x08, 10);
    c.trainer.trainerName = decodeGen3String(raw100 + 0x14, 7);
    c.trainer.trainerId = static_cast<uint16_t>(otId & 0xFFFF);
    c.trainer.secretId = static_cast<uint16_t>((otId >> 16) & 0xFFFF);

    uint8_t data80[80];
    std::memcpy(data80, raw100, 80);

    // Decrypt & Unshuffle Substructures into canonical G, A, E, M order
    decryptPokemonData(data80, pid, otId);

    const uint8_t* blockG = data80 + 0x20; // Block 0: Growth
    const uint8_t* blockA = data80 + 0x2C; // Block 1: Attacks
    const uint8_t* blockE = data80 + 0x38; // Block 2: EVs & Condition
    const uint8_t* blockM = data80 + 0x44; // Block 3: Misc

    c.speciesId = *reinterpret_cast<const uint16_t*>(blockG + 0);
    if (c.speciesId > 0 && c.speciesId < sizeof(kGen3SpeciesNames)/sizeof(kGen3SpeciesNames[0])) {
        c.speciesName = kGen3SpeciesNames[c.speciesId];
    } else {
        c.speciesName = "Species #" + std::to_string(c.speciesId);
    }

    c.heldItemId = *reinterpret_cast<const uint16_t*>(blockG + 2);
    c.experience = *reinterpret_cast<const uint32_t*>(blockG + 4);
    c.friendship.setRawValue(blockG[9]);

    // Moves
    for (int i = 0; i < 4; ++i) {
        uint16_t moveId = *reinterpret_cast<const uint16_t*>(blockA + (i * 2));
        if (moveId > 0) {
            CreatureMove m;
            m.moveId = moveId;
            m.moveName = "Move #" + std::to_string(moveId);
            m.currentPp = blockA[8 + i];
            m.maxPp = 35;
            c.moves.push_back(m);
        }
    }

    // EVs
    c.evs.hp        = blockE[0];
    c.evs.attack    = blockE[1];
    c.evs.defense   = blockE[2];
    c.evs.speed     = blockE[3];
    c.evs.spAttack  = blockE[4];
    c.evs.spDefense = blockE[5];

    // IVs (packed bitfield in Block M offset 4-7)
    uint32_t ivData = *reinterpret_cast<const uint32_t*>(blockM + 4);
    c.ivs.hp        = static_cast<uint8_t>((ivData >> 0) & 0x1F);
    c.ivs.attack    = static_cast<uint8_t>((ivData >> 5) & 0x1F);
    c.ivs.defense   = static_cast<uint8_t>((ivData >> 10) & 0x1F);
    c.ivs.speed     = static_cast<uint8_t>((ivData >> 15) & 0x1F);
    c.ivs.spAttack  = static_cast<uint8_t>((ivData >> 20) & 0x1F);
    c.ivs.spDefense = static_cast<uint8_t>((ivData >> 25) & 0x1F);

    // Derived Nature: PID % 25
    c.nature = static_cast<CreatureNature>(pid % 25);
    c.isDerivedNature = true;

    // Unencrypted Battle Status level (if party 100-byte structure)
    c.level = raw100[0x54];
    if (c.level == 0 || c.level > 100) {
        c.level = 5; // Fallback level calculation
    }

    return c;
}

Gen3SaveParser::SlotInfo Gen3SaveParser::validateAndMapSlot(const uint8_t* slotData, int slotIdx) {
    SlotInfo info;
    info.slotIndex = slotIdx;
    info.isValid = true;

    for (int secIdx = 0; secIdx < 14; ++secIdx) {
        const uint8_t* secPtr = slotData + (secIdx * 4096);

        uint16_t sectionId = *reinterpret_cast<const uint16_t*>(secPtr + 0xFF4);
        uint16_t storedChecksum = *reinterpret_cast<const uint16_t*>(secPtr + 0xFF6);
        uint32_t saveCounter = *reinterpret_cast<const uint32_t*>(secPtr + 0xFFC);

        uint16_t computedChecksum = calculateSectionChecksum(secPtr);
        if (storedChecksum != computedChecksum) {
            info.isValid = false;
        }

        if (sectionId < 14) {
            info.sectionPointers[sectionId] = secPtr;
        }

        if (secIdx == 0) {
            info.saveCounter = saveCounter;
        }
    }

    return info;
}

SaveParseResult Gen3SaveParser::parseSaveFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        SaveParseResult result;
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Could not open save file at " + filePath;
        return result;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < 131072) { // Gen 3 save requires 128KB Flash
        SaveParseResult result;
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Save file size is too small (" + std::to_string(fileSize) + " bytes, expected 128KB)";
        return result;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(131072);
    file.read(reinterpret_cast<char*>(buffer.data()), 131072);

    return parseSaveBuffer(buffer);
}

SaveParseResult Gen3SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) {
    SaveParseResult result;
    if (buffer.size() < 131072) {
        result.status = SaveParseStatus::InvalidFileLength;
        result.errorMessage = "Buffer smaller than 128KB";
        return result;
    }

    SlotInfo slotA = validateAndMapSlot(buffer.data() + 0x00000, 0);
    SlotInfo slotB = validateAndMapSlot(buffer.data() + 0x0E000, 1);

    const SlotInfo* activeSlot = nullptr;
    if (slotA.isValid && slotB.isValid) {
        activeSlot = (slotA.saveCounter >= slotB.saveCounter) ? &slotA : &slotB;
    } else if (slotA.isValid) {
        activeSlot = &slotA;
    } else if (slotB.isValid) {
        activeSlot = &slotB;
    }

    if (!activeSlot) {
        result.status = SaveParseStatus::NoValidSlotFound;
        result.errorMessage = "Both Slot A and Slot B checksums failed validation.";
        return result;
    }

    result.status = SaveParseStatus::Success;
    result.activeSlotIndex = activeSlot->slotIndex;
    result.saveCounter = activeSlot->saveCounter;

    // Parse Section 0: Trainer Info
    const uint8_t* sec0 = activeSlot->sectionPointers[0];
    if (sec0) {
        result.trainerName = decodeGen3String(sec0 + 0x00, 7);
        result.trainerId = *reinterpret_cast<const uint16_t*>(sec0 + 0x0A);
        result.secretId = *reinterpret_cast<const uint16_t*>(sec0 + 0x0C);
        result.playTimeHours = *reinterpret_cast<const uint16_t*>(sec0 + 0x0E);
        result.playTimeMinutes = sec0[0x10];
    }

    // Parse Section 1: Party Pokémon
    const uint8_t* sec1 = activeSlot->sectionPointers[1];
    if (sec1) {
        uint32_t partyCount = *reinterpret_cast<const uint32_t*>(sec1 + 0x234);
        if (partyCount > 6) partyCount = 6;

        for (uint32_t i = 0; i < partyCount; ++i) {
            const uint8_t* pkmnPtr = sec1 + 0x238 + (i * 100);
            Creature pkmn = parsePokemonStruct(pkmnPtr, "Party Slot " + std::to_string(i + 1));
            result.party.push_back(pkmn);
        }
    }

    return result;
}

} // namespace Pocket::Save
