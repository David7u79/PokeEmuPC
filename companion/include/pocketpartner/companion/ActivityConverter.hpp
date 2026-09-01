#pragma once

#include "pocketpartner/companion/CompanionManager.hpp"
#include "pocketpartner/save/SaveMutationPipeline.hpp"
#include <memory>

namespace PocketPartner::Companion {

struct ConversionResult {
    bool applied{false};
    std::string message;
    Save::MutationResult mutationDetails;
};

class ActivityConverter {
public:
    explicit ActivityConverter(std::shared_ptr<Save::SaveMutationPipeline> pipeline);

    ConversionResult convertStreakToFriendship(const CompanionManager& companion,
                                                const std::string& saveFilePath,
                                                bool isEmulatorRunning);

    ConversionResult convertPlayXpToEv(const CompanionManager& companion,
                                        const std::string& saveFilePath,
                                        uint8_t targetStatIndex,
                                        bool isEmulatorRunning);

private:
    std::shared_ptr<Save::SaveMutationPipeline> m_pipeline;
};

} // namespace PocketPartner::Companion
