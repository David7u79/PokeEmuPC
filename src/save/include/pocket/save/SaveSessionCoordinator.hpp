#pragma once

#include <string>
#include <map>
#include <mutex>
#include <functional>
#include "pocket/save/SaveSessionState.hpp"

namespace Pocket::Save {

class SaveSessionCoordinator {
public:
    SaveSessionCoordinator() = default;

    SaveSessionState getState(const std::string& savePath) const;

    bool acquireEmulatorLock(const std::string& savePath);
    bool releaseEmulatorLock(const std::string& savePath);

    bool acquireMutationLock(const std::string& savePath);
    bool releaseMutationLock(const std::string& savePath);

    bool setExternalProcessActive(const std::string& savePath, bool active);

    bool canMutateSave(const std::string& savePath) const;
    bool canReadSave(const std::string& savePath) const;

    using StateChangedCallback = std::function<void(const std::string& savePath, SaveSessionState newState)>;
    void setStateChangedCallback(StateChangedCallback callback) { m_callback = std::move(callback); }

private:
    void updateState(const std::string& savePath, SaveSessionState newState);

    mutable std::mutex m_mutex;
    std::map<std::string, SaveSessionState> m_sessionStates;
    StateChangedCallback m_callback;
};

} // namespace Pocket::Save
