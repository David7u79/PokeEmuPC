#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/save/CreatureSaveParser.hpp"

namespace Pocket::Save {

enum class EditorStatus {
    Success,
    SaveLockedByEmulator,
    SaveHashMismatch,
    CreatureNotFound,
    CreatureAmbiguous,
    ChecksumRepairFailed,
    AtomicWriteFailed,
    SemanticDiffFailed
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
        default:                                 return "Unknown Error";
    }
}

struct MutationAudit {
    std::string creatureNickname;
    std::string speciesName;
    uint8_t oldFriendship{0};
    uint8_t newFriendship{0};

    size_t bytesModified{0}; // Should be exactly 3 bytes (1 friendship + 2 section checksum)
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
};

} // namespace Pocket::Save
