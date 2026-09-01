#pragma once

#include "pocket/save/CreatureSaveEditor.hpp"
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/save/SaveSessionCoordinator.hpp"
#include "pocket/save/SaveBackupRepository.hpp"
#include "pocket/save/FileStabilityVerifier.hpp"

namespace Pocket::Save {

class Gen3SaveEditor : public CreatureSaveEditor {
public:
    explicit Gen3SaveEditor(
        std::shared_ptr<SaveSessionCoordinator> coordinator = std::make_shared<SaveSessionCoordinator>(),
        std::shared_ptr<SaveBackupRepository> backupRepo = std::make_shared<SaveBackupRepository>()
    );

    MutationResult mutateFriendship(
        const std::string& saveFilePath,
        const Pocket::Companion::CompanionLink& targetLink,
        uint8_t newFriendshipValue
    ) override;

    static std::string calculateSha256(const std::vector<uint8_t>& buffer);

private:
    std::shared_ptr<SaveSessionCoordinator> m_coordinator;
    std::shared_ptr<SaveBackupRepository> m_backupRepo;
    Gen3SaveParser m_parser;
};

} // namespace Pocket::Save
