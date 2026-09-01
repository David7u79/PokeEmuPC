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

inline std::string saveSessionStateToString(SaveSessionState state) {
    switch (state) {
        case SaveSessionState::Available:             return "Available";
        case SaveSessionState::EmulatorActive:        return "EmulatorActive";
        case SaveSessionState::ExternalProcessActive:return "ExternalProcessActive";
        case SaveSessionState::MutationActive:       return "MutationActive";
        case SaveSessionState::UnknownBusy:          return "UnknownBusy";
        default:                                     return "Unknown";
    }
}

} // namespace Pocket::Save
