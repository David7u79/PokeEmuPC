#pragma once

#include "pocketpartner/save/SaveFileParser.hpp"
#include "pocketpartner/core/CompanionLink.hpp"
#include "pocketpartner/core/CanonicalPokemonState.hpp"
#include <string>
#include <functional>
#include <memory>

namespace PocketPartner::Save {

enum class MutationType {
    AddEv,
    IncreaseFriendship
};

struct MutationRequest {
    Core::CompanionLink targetLink;
    MutationType type;
    uint16_t parameterValue{0};
    uint8_t targetStatIndex{0}; // 0: HP, 1: Atk, 2: Def, 3: Spd, 4: SpAtk, 5: SpDef
};

struct MutationResult {
    bool success{false};
    std::string errorMessage;
    uint32_t stepFailed{0}; // 1 to 12
    Core::CanonicalPokemonState previousState;
    Core::CanonicalPokemonState newState;
    std::string backupFilePath;
};

class SaveMutationPipeline {
public:
    explicit SaveMutationPipeline(std::shared_ptr<SaveFileParser> parser);

    // Executes the strict 12-step save safety mutation protocol
    MutationResult executeMutation(const std::string& saveFilePath,
                                    const MutationRequest& request,
                                    bool isEmulatorRunning = false);

private:
    std::shared_ptr<SaveFileParser> m_parser;

    uint64_t computeBufferHash(const std::vector<uint8_t>& buffer) const;
    bool isFileLocked(const std::string& path) const;
};

} // namespace PocketPartner::Save
