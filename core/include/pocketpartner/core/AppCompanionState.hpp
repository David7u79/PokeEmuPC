#pragma once

#include <string>
#include <cstdint>
#include <algorithm>

namespace PocketPartner::Core {

struct AppCompanionState {
    std::string companionId;
    double hunger{100.0};       // 0.0 (starving) to 100.0 (full)
    double mood{100.0};         // 0.0 (sad) to 100.0 (ecstatic)
    double fatigue{0.0};        // 0.0 (energetic) to 100.0 (exhausted)
    double cleanliness{100.0};  // 0.0 (dirty) to 100.0 (spotless)
    uint32_t bondLevel{1};
    uint32_t streakDays{0};
    int64_t lastInteractionTs{0};
    uint64_t companionXp{0};
    std::string cosmeticState{"default"};
    std::string animationState{"idle"};

    void clampValues() {
        hunger = std::clamp(hunger, 0.0, 100.0);
        mood = std::clamp(mood, 0.0, 100.0);
        fatigue = std::clamp(fatigue, 0.0, 100.0);
        cleanliness = std::clamp(cleanliness, 0.0, 100.0);
    }

    // Lazily update state based on elapsed time without running timer polling loops
    void updateElapsed(int64_t currentTimestampSecs) {
        if (lastInteractionTs <= 0) {
            lastInteractionTs = currentTimestampSecs;
            return;
        }

        int64_t deltaSecs = currentTimestampSecs - lastInteractionTs;
        if (deltaSecs <= 0) return;

        // Decay rates per hour (3600 seconds)
        double hours = static_cast<double>(deltaSecs) / 3600.0;
        hunger -= hours * 4.0;       // -4% hunger per hour
        fatigue += hours * 2.5;      // +2.5% fatigue per hour
        cleanliness -= hours * 1.5;  // -1.5% cleanliness per hour
        mood -= hours * 3.0;         // -3.0% mood per hour

        clampValues();
        lastInteractionTs = currentTimestampSecs;
    }
};

} // namespace PocketPartner::Core
