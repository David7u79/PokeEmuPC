#pragma once

#include "pocketpartner/core/AppCompanionState.hpp"
#include "pocketpartner/core/CompanionLink.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include <memory>

namespace PocketPartner::Companion {

enum class ActivityType {
    Feed,
    Play,
    Pet,
    Clean,
    Rest
};

class CompanionManager {
public:
    CompanionManager(Core::CompanionLink activeLink,
                     std::shared_ptr<Storage::DatabaseManager> db);

    bool loadState();
    bool saveState();

    void updateLazyState(int64_t currentTimestampSecs);
    bool interact(ActivityType activity, int64_t currentTimestampSecs);

    const Core::AppCompanionState& state() const { return m_state; }
    const Core::CompanionLink& activeLink() const { return m_link; }

private:
    Core::CompanionLink m_link;
    Core::AppCompanionState m_state;
    std::shared_ptr<Storage::DatabaseManager> m_db;
};

} // namespace PocketPartner::Companion
