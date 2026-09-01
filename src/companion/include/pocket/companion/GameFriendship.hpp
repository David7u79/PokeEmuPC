#pragma once

#include <cstdint>
#include <algorithm>

namespace Pocket::Companion {

// GameFriendship represents canonical game save friendship byte (0..255)
// STRICTLY DECOUPLED from app-only CompanionBond to prevent accidental confusion
struct GameFriendship {
    uint8_t value{70}; // Default base friendship for imported Pokémon

    void setRawValue(uint8_t val) {
        value = val;
    }

    uint8_t rawValue() const {
        return value;
    }
};

} // namespace Pocket::Companion
