#include "pocket/save/PendingGameReward.hpp"
#include <algorithm>

namespace Pocket::Save {

static bool isSameDay(uint64_t t1, uint64_t t2) {
    // 86400 seconds in a day
    return (t1 / 86400) == (t2 / 86400);
}

int PendingRewardLedger::getDailySessionCount(int companionLinkId, uint64_t nowTimestamp) const {
    int count = 0;
    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && isSameDay(r.timestamp, nowTimestamp)) {
            count++;
        }
    }
    return count;
}

bool PendingRewardLedger::canAddReward(
    int companionLinkId,
    RewardCategory category,
    int requestedAmount,
    uint64_t nowTimestamp,
    std::string& outReason
) const {
    // 1. Check Cooldown
    uint64_t lastTimestamp = 0;
    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && r.timestamp > lastTimestamp) {
            lastTimestamp = r.timestamp;
        }
    }

    if (lastTimestamp > 0 && (nowTimestamp - lastTimestamp) < static_cast<uint64_t>(m_rules.cooldownSeconds)) {
        outReason = "Action is on cooldown. Please wait " + std::to_string(m_rules.cooldownSeconds - (nowTimestamp - lastTimestamp)) + " seconds.";
        return false;
    }

    // 2. Check Daily Caps
    int dailyEvSum = 0;
    int dailyFriendshipSum = 0;

    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && isSameDay(r.timestamp, nowTimestamp)) {
            if (r.category == RewardCategory::EV) {
                dailyEvSum += r.amount;
            } else if (r.category == RewardCategory::Friendship) {
                dailyFriendshipSum += r.amount;
            }
        }
    }

    if (category == RewardCategory::EV && (dailyEvSum + requestedAmount) > m_rules.maxDailyEvPoints) {
        outReason = "Daily EV point cap (" + std::to_string(m_rules.maxDailyEvPoints) + " pts) reached for today.";
        return false;
    }

    if (category == RewardCategory::Friendship && (dailyFriendshipSum + requestedAmount) > m_rules.maxDailyFriendshipPoints) {
        outReason = "Daily Friendship point cap (" + std::to_string(m_rules.maxDailyFriendshipPoints) + " pts) reached for today.";
        return false;
    }

    return true;
}

int PendingRewardLedger::calculateAdjustedAmount(
    int companionLinkId,
    RewardCategory category,
    int baseAmount,
    uint64_t nowTimestamp
) const {
    int sessionCount = getDailySessionCount(companionLinkId, nowTimestamp);
    int adjusted = baseAmount;

    // Apply Diminishing Returns if session threshold is passed
    if (sessionCount >= m_rules.diminishingReturnsThreshold) {
        adjusted = std::max(1, baseAmount / 2);
    }

    // Clamp to remaining daily cap
    int dailySum = 0;
    int maxCap = (category == RewardCategory::EV) ? m_rules.maxDailyEvPoints : m_rules.maxDailyFriendshipPoints;

    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && r.category == category && isSameDay(r.timestamp, nowTimestamp)) {
            dailySum += r.amount;
        }
    }

    int remainingCap = std::max(0, maxCap - dailySum);
    return std::min(adjusted, remainingCap);
}

bool PendingRewardLedger::recordReward(const PendingGameReward& reward, std::string& outReason) {
    if (!canAddReward(reward.companionLinkId, reward.category, reward.amount, reward.timestamp, outReason)) {
        return false;
    }

    PendingGameReward entry = reward;
    entry.rewardId = m_nextRewardId++;
    entry.amount = calculateAdjustedAmount(reward.companionLinkId, reward.category, reward.amount, reward.timestamp);

    m_rewards.push_back(entry);
    return true;
}

std::vector<PendingGameReward> PendingRewardLedger::getPendingRewards(int companionLinkId) const {
    std::vector<PendingGameReward> result;
    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && !r.isApplied) {
            result.push_back(r);
        }
    }
    return result;
}

int PendingRewardLedger::getTotalPendingEV(int companionLinkId, EVType evStat) const {
    int total = 0;
    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && !r.isApplied && r.category == RewardCategory::EV && r.evStat == evStat) {
            total += r.amount;
        }
    }
    return total;
}

int PendingRewardLedger::getTotalPendingFriendship(int companionLinkId) const {
    int total = 0;
    for (const auto& r : m_rewards) {
        if (r.companionLinkId == companionLinkId && !r.isApplied && r.category == RewardCategory::Friendship) {
            total += r.amount;
        }
    }
    return total;
}

} // namespace Pocket::Save
