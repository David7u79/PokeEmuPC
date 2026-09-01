#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Pocket::Save {

enum class EVType {
    HP,
    Attack,
    Defense,
    SpecialAttack,
    SpecialDefense,
    Speed
};

inline std::string evTypeToString(EVType ev) {
    switch (ev) {
        case EVType::HP:             return "HP";
        case EVType::Attack:         return "Attack";
        case EVType::Defense:        return "Defense";
        case EVType::SpecialAttack:  return "Special Attack";
        case EVType::SpecialDefense: return "Special Defense";
        case EVType::Speed:          return "Speed";
        default:                     return "Unknown";
    }
}

enum class RewardCategory {
    Friendship,
    EV
};

struct PendingGameReward {
    int rewardId{0};
    int companionLinkId{0};
    RewardCategory category{RewardCategory::EV};
    EVType evStat{EVType::Attack};
    int amount{0};
    uint64_t timestamp{0}; // epoch seconds
    std::string sourceAction; // "Train_TimingBar", "Feed", "Pet", "Play"
    bool isApplied{false};
};

struct TrainingRewardRules {
    int cooldownSeconds{30};
    int maxDailyEvPoints{20};
    int maxDailyFriendshipPoints{15};
    int diminishingReturnsThreshold{5};
};

class PendingRewardLedger {
public:
    PendingRewardLedger() = default;

    void setRules(const TrainingRewardRules& rules) { m_rules = rules; }
    const TrainingRewardRules& rules() const { return m_rules; }

    bool canAddReward(
        int companionLinkId,
        RewardCategory category,
        int requestedAmount,
        uint64_t nowTimestamp,
        std::string& outReason
    ) const;

    int calculateAdjustedAmount(
        int companionLinkId,
        RewardCategory category,
        int baseAmount,
        uint64_t nowTimestamp
    ) const;

    bool recordReward(const PendingGameReward& reward, std::string& outReason);

    std::vector<PendingGameReward> getPendingRewards(int companionLinkId) const;
    int getTotalPendingEV(int companionLinkId, EVType evStat) const;
    int getTotalPendingFriendship(int companionLinkId) const;
    int getDailySessionCount(int companionLinkId, uint64_t nowTimestamp) const;

    void clearAll() { m_rewards.clear(); }

private:
    TrainingRewardRules m_rules;
    std::vector<PendingGameReward> m_rewards;
    int m_nextRewardId{1};
};

} // namespace Pocket::Save
