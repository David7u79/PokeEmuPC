#pragma once

#include <cstdint>
#include <string>

namespace Pocket::Companion {

// CompanionBond is app-only and distinct from canonical GameFriendship
struct CompanionBond {
    uint32_t level{1};
    uint64_t xp{0};
    uint32_t streakDays{0};

    void addXp(uint32_t amount) {
        xp += amount;
        uint32_t requiredXp = level * 100;
        while (xp >= requiredXp) {
            xp -= requiredXp;
            level++;
            requiredXp = level * 100;
        }
    }

    std::string displayName() const {
        return "Bond Lv. " + std::to_string(level);
    }
};

} // namespace Pocket::Companion
