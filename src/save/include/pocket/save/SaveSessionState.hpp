#pragma once

#include <string>

namespace Pocket::Save {

enum class SaveSessionState {
    Available,
    EmulatorActive,
    ExternalProcessActive,
    MutationActive,
    UnknownBusy
};

enum class ExternalSyncMode {
    ReadOnlyExternal,
    SafeExternalMutation
};

inline std::string saveSessionStateToString(SaveSessionState state) {
    switch (state) {
        case SaveSessionState::Available:             return "Available";
        case SaveSessionState::EmulatorActive:        return "EmulatorActive";
        case SaveSessionState::ExternalProcessActive: return "ExternalProcessActive";
        case SaveSessionState::MutationActive:        return "MutationActive";
        case SaveSessionState::UnknownBusy:           return "UnknownBusy";
        default:                                      return "Unknown";
    }
}

inline std::string externalSyncModeToString(ExternalSyncMode mode) {
    switch (mode) {
        case ExternalSyncMode::ReadOnlyExternal:    return "ReadOnlyExternal";
        case ExternalSyncMode::SafeExternalMutation: return "SafeExternalMutation";
        default:                                     return "Unknown";
    }
}

} // namespace Pocket::Save
