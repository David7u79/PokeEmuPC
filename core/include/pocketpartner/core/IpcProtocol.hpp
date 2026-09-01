#pragma once

#include <string>
#include <cstdint>
#include "pocketpartner/core/AppCompanionState.hpp"
#include "pocketpartner/core/CanonicalPokemonState.hpp"

namespace PocketPartner::Core {

enum class IpcMessageType : uint8_t {
    Ping = 1,
    Pong = 2,
    CompanionStateSync = 10,
    CanonicalStateSync = 11,
    ActivityPerformed = 20,
    EmulatorStatusChanged = 30,
    SaveMutationRequest = 40,
    SaveMutationResponse = 41
};

struct IpcMessage {
    IpcMessageType type{IpcMessageType::Ping};
    uint64_t sequenceNumber{0};
    std::string payloadJson;
};

} // namespace PocketPartner::Core
