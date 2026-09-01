#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace PocketPartner::Core {

struct StatEffortValues {
    uint8_t hp{0};
    uint8_t attack{0};
    uint8_t defense{0};
    uint8_t speed{0};
    uint8_t spAttack{0};
    uint8_t spDefense{0};

    uint16_t totalEvs() const {
        return static_cast<uint16_t>(hp) + attack + defense + speed + spAttack + spDefense;
    }

    bool isValidGen3Plus() const {
        return totalEvs() <= 510 &&
               hp <= 252 && attack <= 252 && defense <= 252 &&
               speed <= 252 && spAttack <= 252 && spDefense <= 252;
    }
};

struct StatIndividualValues {
    uint8_t hp{0};
    uint8_t attack{0};
    uint8_t defense{0};
    uint8_t speed{0};
    uint8_t spAttack{0};
    uint8_t spDefense{0};
};

struct TrainerInfo {
    uint16_t trainerId{0};
    uint16_t secretId{0};
    std::string name;
    uint8_t gender{0};
    uint8_t originGame{0};
    uint8_t pokeball{0};
};

struct CanonicalPokemonState {
    uint16_t speciesId{0};
    std::string nickname;
    uint8_t level{1};
    uint32_t experience{0};
    uint8_t friendship{0};
    StatEffortValues evs;
    StatIndividualValues ivs;
    uint8_t nature{0};
    std::array<uint16_t, 4> moves{0, 0, 0, 0};
    uint16_t heldItem{0};
    TrainerInfo trainer;
    uint32_t personalityValue{0};

    bool isValid() const {
        return speciesId > 0 && level >= 1 && level <= 100 && evs.isValidGen3Plus();
    }
};

} // namespace PocketPartner::Core
