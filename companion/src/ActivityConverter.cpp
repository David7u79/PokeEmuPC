#include "pocketpartner/companion/ActivityConverter.hpp"

namespace PocketPartner::Companion {

ActivityConverter::ActivityConverter(std::shared_ptr<Save::SaveMutationPipeline> pipeline)
    : m_pipeline(std::move(pipeline)) {}

ConversionResult ActivityConverter::convertStreakToFriendship(const CompanionManager& companion,
                                                               const std::string& saveFilePath,
                                                               bool isEmulatorRunning) {
    ConversionResult res;
    if (!m_pipeline) {
        res.message = "Null mutation pipeline.";
        return res;
    }

    Save::MutationRequest req;
    req.targetLink = companion.activeLink();
    req.type = Save::MutationType::IncreaseFriendship;
    req.parameterValue = 5; // Reward +5 friendship for activity milestone

    res.mutationDetails = m_pipeline->executeMutation(saveFilePath, req, isEmulatorRunning);
    res.applied = res.mutationDetails.success;
    res.message = res.mutationDetails.errorMessage;
    return res;
}

ConversionResult ActivityConverter::convertPlayXpToEv(const CompanionManager& companion,
                                                       const std::string& saveFilePath,
                                                       uint8_t targetStatIndex,
                                                       bool isEmulatorRunning) {
    ConversionResult res;
    if (!m_pipeline) {
        res.message = "Null mutation pipeline.";
        return res;
    }

    Save::MutationRequest req;
    req.targetLink = companion.activeLink();
    req.type = Save::MutationType::AddEv;
    req.parameterValue = 2; // Reward +2 EV points
    req.targetStatIndex = targetStatIndex;

    res.mutationDetails = m_pipeline->executeMutation(saveFilePath, req, isEmulatorRunning);
    res.applied = res.mutationDetails.success;
    res.message = res.mutationDetails.errorMessage;
    return res;
}

} // namespace PocketPartner::Companion
