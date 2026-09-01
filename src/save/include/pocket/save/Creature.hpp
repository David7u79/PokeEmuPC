#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "pocket/companion/GameFriendship.hpp"

namespace Pocket::Save {

enum class GenerationType {
    Gen1 = 1,
    Gen2 = 2,
    Gen3 = 3
};

inline std::string generationTypeToString(GenerationType gen) {
    switch (gen) {
        case GenerationType::Gen1: return "Generation I (Red/Blue/Yellow)";
        case GenerationType::Gen2: return "Generation II (Gold/Silver/Crystal)";
        case GenerationType::Gen3: return "Generation III (GBA)";
        default:                   return "Unknown Generation";
    }
}

// Gen I & II Stat Experience (16-bit 0..65535 per stat)
struct StatExp {
    uint16_t hp{0};
    uint16_t attack{0};
    uint16_t defense{0};
    uint16_t speed{0};
    uint16_t special{0};
};

// Gen I & II Determinant Values (4-bit 0..15 per stat)
struct DVs {
    uint8_t hp{0};
    uint8_t attack{0};
    uint8_t defense{0};
    uint8_t speed{0};
    uint8_t special{0};
};

struct CreatureEVs {
    uint8_t hp{0};
    uint8_t attack{0};
    uint8_t defense{0};
    uint8_t spAttack{0};
    uint8_t spDefense{0};
    uint8_t speed{0};
};

struct CreatureIVs {
    uint8_t hp{0};
    uint8_t attack{0};
    uint8_t defense{0};
    uint8_t spAttack{0};
    uint8_t spDefense{0};
    uint8_t speed{0};
};

enum class CreatureNature {
    Hardy = 0,   Lonely,   Brave,    Adamant,  Naughty,
    Bold,        Docile,   Relaxed,  Impish,   Lax,
    Timid,       Hasty,    Serious,  Jolly,    Naive,
    Modest,      Mild,     Quiet,    Bashful,  Rash,
    Calm,        Gentle,   Sassy,    Careful,  Quirky
};

inline std::string natureToString(CreatureNature nature) {
    switch (nature) {
        case CreatureNature::Hardy:   return "Hardy";
        case CreatureNature::Lonely:  return "Lonely";
        case CreatureNature::Brave:   return "Brave";
        case CreatureNature::Adamant: return "Adamant";
        case CreatureNature::Naughty: return "Naughty";
        case CreatureNature::Bold:    return "Bold";
        case CreatureNature::Docile:  return "Docile";
        case CreatureNature::Relaxed: return "Relaxed";
        case CreatureNature::Impish:  return "Impish";
        case CreatureNature::Lax:     return "Lax";
        case CreatureNature::Timid:   return "Timid";
        case CreatureNature::Hasty:   return "Hasty";
        case CreatureNature::Serious: return "Serious";
        case CreatureNature::Jolly:   return "Jolly";
        case CreatureNature::Naive:   return "Naive";
        case CreatureNature::Modest:  return "Modest";
        case CreatureNature::Mild:    return "Mild";
        case CreatureNature::Quiet:   return "Quiet";
        case CreatureNature::Bashful: return "Bashful";
        case CreatureNature::Rash:    return "Rash";
        case CreatureNature::Calm:    return "Calm";
        case CreatureNature::Gentle:  return "Gentle";
        case CreatureNature::Sassy:   return "Sassy";
        case CreatureNature::Careful: return "Careful";
        case CreatureNature::Quirky:  return "Quirky";
        default:                      return "Unknown";
    }
}

struct CreatureTrainer {
    std::string trainerName;
    uint16_t trainerId{0};
    uint16_t secretId{0};
    bool isFemale{false};
};

struct CreatureMove {
    uint16_t moveId{0};
    std::string moveName;
    uint8_t currentPp{0};
    uint8_t maxPp{0};
};

struct Creature {
    GenerationType generation{GenerationType::Gen3};
    uint16_t speciesId{0};
    std::string speciesName;
    std::string nickname;

    uint8_t level{1};
    uint32_t experience{0};
    Pocket::Companion::GameFriendship friendship;
    bool hasFriendship{true};

    // Gen III EV/IVs
    CreatureEVs evs;
    CreatureIVs ivs;

    // Gen I / II Stat Exp & DVs
    StatExp statExp;
    DVs dvs;

    CreatureNature nature{CreatureNature::Hardy};

    std::vector<CreatureMove> moves;
    uint16_t heldItemId{0};
    std::string heldItemName;

    CreatureTrainer trainer;
    uint32_t personalityValue{0};
    std::string location;

    bool isDerivedNature{true};
};

} // namespace Pocket::Save
