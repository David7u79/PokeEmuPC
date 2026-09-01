#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/save/CreatureSaveParser.hpp"
#include "pocket/save/PendingGameReward.hpp"

namespace Pocket::Save {

enum class EditorStatus {
    Success,
    SaveLockedByEmulator,
    SaveHashMismatch,
    CreatureNotFound,
    CreatureAmbiguous,
    ChecksumRepairFailed,
    AtomicWriteFailed,
    SemanticDiffFailed,
    CapExceededNoGain
};

inline std::string editorStatusToString(EditorStatus status) {
    switch (status) {
        case EditorStatus::Success:              return "Success";
        case EditorStatus::SaveLockedByEmulator: return "Save Locked By Emulator";
        case EditorStatus::SaveHashMismatch:     return "Save Hash Mismatch (Stale Save)";
        case EditorStatus::CreatureNotFound:     return "Creature Not Found in Save";
        case EditorStatus::CreatureAmbiguous:    return "Creature Ambiguous (Multiple Candidates)";
        case EditorStatus::ChecksumRepairFailed: return "Checksum Repair Failed";
        case EditorStatus::AtomicWriteFailed:    return "Atomic File Write Failed";
        case EditorStatus::SemanticDiffFailed:   return "Semantic Diff Verification Failed";
        case EditorStatus::CapExceededNoGain:    return "EV Cap Exceeded (No Gain Applied)";
        default:                                 return "Unknown Error";
    }
}

struct MutationAudit {
    std::string creatureNickname;
    std::string speciesName;
    uint8_t oldFriendship{0};
    uint8_t newFriendship{0};

    EVType evStat{EVType::Attack};
    uint8_t oldEvValue{0};
    uint8_t newEvValue{0};
    int requestedEvAmount{0};
    int appliedEvAmount{0};
    int remainingEvAmount{0};

    size_t bytesModified{0};
    size_t unrelatedFieldsChanged{0}; // Should be 0
    bool isVerified{false};
    std::string backupFilePath;
};

struct MutationResult {
    EditorStatus status{EditorStatus::SemanticDiffFailed};
    std::string errorMessage;
    MutationAudit audit;
    std::string newSaveHash;
};

class CreatureSaveEditor {
public:
    virtual ~CreatureSaveEditor() = default;

    virtual MutationResult mutateFriendship(
        const std::string& saveFilePath,
        const Pocket::Companion::CompanionLink& targetLink,
        uint8_t newFriendshipValue
    ) = 0;

    virtual MutationResult mutateEV(
        const std::string& saveFilePath,
        const Pocket::Companion::CompanionLink& targetLink,
        EVType evStat,
        int requestedEvAmount
    ) = 0;
};

} // namespace Pocket::Save
