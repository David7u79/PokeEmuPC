#include "pocket/save/SaveSessionCoordinator.hpp"

namespace Pocket::Save {

SaveSessionState SaveSessionCoordinator::getState(const std::string& savePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessionStates.find(savePath);
    if (it != m_sessionStates.end()) {
        return it->second;
    }
    return SaveSessionState::Available;
}

bool SaveSessionCoordinator::acquireEmulatorLock(const std::string& savePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SaveSessionState current = m_sessionStates[savePath];

    if (current == SaveSessionState::Available) {
        updateState(savePath, SaveSessionState::EmulatorActive);
        return true;
    }
    return false; // Cannot acquire lock if mutation or another process is active
}

bool SaveSessionCoordinator::releaseEmulatorLock(const std::string& savePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SaveSessionState current = m_sessionStates[savePath];

    if (current == SaveSessionState::EmulatorActive) {
        updateState(savePath, SaveSessionState::Available);
        return true;
    }
    return false;
}

bool SaveSessionCoordinator::acquireMutationLock(const std::string& savePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SaveSessionState current = m_sessionStates[savePath];

    // Rule: While emulator has loaded a game save, Companion canonical save mutation is forbidden!
    if (current == SaveSessionState::Available) {
        updateState(savePath, SaveSessionState::MutationActive);
        return true;
    }
    return false;
}

bool SaveSessionCoordinator::releaseMutationLock(const std::string& savePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SaveSessionState current = m_sessionStates[savePath];

    if (current == SaveSessionState::MutationActive) {
        updateState(savePath, SaveSessionState::Available);
        return true;
    }
    return false;
}

bool SaveSessionCoordinator::setExternalProcessActive(const std::string& savePath, bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (active) {
        if (m_sessionStates[savePath] == SaveSessionState::Available) {
            updateState(savePath, SaveSessionState::ExternalProcessActive);
            return true;
        }
        return false;
    } else {
        if (m_sessionStates[savePath] == SaveSessionState::ExternalProcessActive) {
            updateState(savePath, SaveSessionState::Available);
            return true;
        }
        return false;
    }
}

bool SaveSessionCoordinator::canMutateSave(const std::string& savePath) const {
    return getState(savePath) == SaveSessionState::Available;
}

bool SaveSessionCoordinator::canReadSave(const std::string& savePath) const {
    SaveSessionState current = getState(savePath);
    return current == SaveSessionState::Available || current == SaveSessionState::EmulatorActive;
}

void SaveSessionCoordinator::updateState(const std::string& savePath, SaveSessionState newState) {
    m_sessionStates[savePath] = newState;
    if (m_callback) {
        m_callback(savePath, newState);
    }
}

} // namespace Pocket::Save
