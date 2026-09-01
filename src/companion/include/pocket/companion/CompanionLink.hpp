#pragma once

#include <string>
#include <cstdint>

namespace Pocket::Companion {

enum class LocationType {
    Party,
    Box,
    Unknown
};

struct CreatureLocator {
    LocationType type{LocationType::Party};
    int partySlot{1};  // 1..6
    int boxNumber{1};  // 1..14
    int boxSlot{1};    // 1..30

    std::string toString() const {
        if (type == LocationType::Party) {
            return "Party Slot " + std::to_string(partySlot);
        } else if (type == LocationType::Box) {
            return "Box " + std::to_string(boxNumber) + " Slot " + std::to_string(boxSlot);
        }
        return "Unknown Location";
    }
};

struct CompanionFingerprint {
    uint32_t personalityValue{0};
    uint16_t trainerId{0};
    uint16_t secretId{0};
    bool isFemale{false};
    std::string otName;

    bool matches(uint32_t pid, uint16_t otId, uint16_t secId) const {
        return (personalityValue == pid && trainerId == otId && secretId == secId);
    }
};

enum class LinkStatus {
    Linked,
    NeedsRelink,
    NotFound,
    AmbiguousMatch
};

inline std::string linkStatusToString(LinkStatus status) {
    switch (status) {
        case LinkStatus::Linked:         return "Linked";
        case LinkStatus::NeedsRelink:    return "Needs Relink";
        case LinkStatus::NotFound:       return "Not Found in Save";
        case LinkStatus::AmbiguousMatch: return "Ambiguous Match";
        default:                         return "Unknown";
    }
}

struct CompanionLink {
    int gameId{0};
    int generation{3};
    CreatureLocator locator;
    CompanionFingerprint fingerprint;
    std::string lastVerifiedSaveHash;
    LinkStatus status{LinkStatus::NotFound};
    uint64_t lastUpdated{0};

    // Cached canonical display attributes
    std::string nickname;
    std::string speciesName;
    uint8_t level{1};
    uint8_t gameFriendship{0};
};

} // namespace Pocket::Companion
