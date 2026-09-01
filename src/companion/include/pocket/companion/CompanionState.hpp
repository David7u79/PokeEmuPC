#pragma once

#include <string>
#include <cstdint>
#include "pocket/companion/CompanionBond.hpp"

namespace Pocket::Companion {

struct CompanionState {
    std::string companionId{"default_partner"};
    std::string displayName{"Partner"};

    double hunger{100.0};  // 0.0 (starving) to 100.0 (full)
    double mood{100.0};    // 0.0 (sad) to 100.0 (happy)
    double energy{100.0};  // 0.0 (exhausted) to 100.0 (energetic)
    double fatigue{0.0};   // 0.0 (rested) to 100.0 (fatigued)

    CompanionBond bond;

    int64_t lastInteractionAt{0};
    int64_t lastFedAt{0};
    int64_t lastPlayedAt{0};
    int64_t lastRestedAt{0};
    int64_t lastCalculatedAt{0};
};

} // namespace Pocket::Companion
