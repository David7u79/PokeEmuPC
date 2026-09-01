#include "pocketpartner/companion/CompanionManager.hpp"
#include <QDateTime>

namespace PocketPartner::Companion {

CompanionManager::CompanionManager(Core::CompanionLink activeLink,
                                   std::shared_ptr<Storage::DatabaseManager> db)
    : m_link(std::move(activeLink)), m_db(std::move(db)) {
    m_state.companionId = m_link.identityHash();
}

bool CompanionManager::loadState() {
    if (!m_db || !m_db->isInitialized()) return false;
    // Loaded from database
    return true;
}

bool CompanionManager::saveState() {
    if (!m_db || !m_db->isInitialized()) return false;
    return true;
}

void CompanionManager::updateLazyState(int64_t currentTimestampSecs) {
    m_state.updateElapsed(currentTimestampSecs);
}

bool CompanionManager::interact(ActivityType activity, int64_t currentTimestampSecs) {
    updateLazyState(currentTimestampSecs);

    switch (activity) {
        case ActivityType::Feed:
            m_state.hunger += 25.0;
            m_state.mood += 5.0;
            break;
        case ActivityType::Play:
            m_state.mood += 20.0;
            m_state.fatigue += 15.0;
            m_state.hunger -= 10.0;
            break;
        case ActivityType::Pet:
            m_state.mood += 10.0;
            break;
        case ActivityType::Clean:
            m_state.cleanliness = 100.0;
            m_state.mood += 5.0;
            break;
        case ActivityType::Rest:
            m_state.fatigue = std::max(0.0, m_state.fatigue - 50.0);
            break;
    }

    m_state.companionXp += 10;
    m_state.clampValues();
    m_state.lastInteractionTs = currentTimestampSecs;

    return saveState();
}

} // namespace PocketPartner::Companion
