#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "pocket/companion/GameFriendship.hpp"

namespace Pocket::Save {

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
    int generation{3};
    uint16_t speciesId{0};
    std::string speciesName;
    std::string nickname;

    uint8_t level{1};
    uint32_t experience{0};
    Pocket::Companion::GameFriendship friendship;

    CreatureEVs evs;
    CreatureIVs ivs;
    CreatureNature nature{CreatureNature::Hardy};

    std::vector<CreatureMove> moves;
    uint16_t heldItemId{0};
    std::string heldItemName;

    CreatureTrainer trainer;
    uint32_t personalityValue{0};
    std::string location; // e.g. "Party Slot 1", "Box 1 Slot 5"

    bool isDerivedNature{true};
};

} // namespace Pocket::Save
