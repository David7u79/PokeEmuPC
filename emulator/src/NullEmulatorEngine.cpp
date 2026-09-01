#include "pocketpartner/emulator/NullEmulatorEngine.hpp"

namespace PocketPartner::Emulator {

std::vector<TargetSystem> NullEmulatorEngine::supportedSystems() const {
    return {
        TargetSystem::GameBoy,
        TargetSystem::GameBoyColor,
        TargetSystem::GameBoyAdvance,
        TargetSystem::NintendoDS
    };
}

Result<bool> NullEmulatorEngine::loadGame(const GameLaunchRequest& request) {
    if (request.romPath.empty()) {
        return Result<bool>::fail("ROM path cannot be empty.");
    }
    m_currentRequest = request;
    m_state = EmulatorState::Loaded;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::start() {
    if (m_state != EmulatorState::Loaded && m_state != EmulatorState::Stopped) {
        return Result<bool>::fail("Cannot start emulator unless loaded or stopped.");
    }
    m_state = EmulatorState::Running;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::pause() {
    if (m_state != EmulatorState::Running) {
        return Result<bool>::fail("Cannot pause emulator unless running.");
    }
    m_state = EmulatorState::Paused;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::resume() {
    if (m_state != EmulatorState::Paused) {
        return Result<bool>::fail("Cannot resume emulator unless paused.");
    }
    m_state = EmulatorState::Running;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::stop() {
    m_state = EmulatorState::Stopped;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::reset() {
    if (m_state == EmulatorState::Unloaded) {
        return Result<bool>::fail("Cannot reset unloaded game.");
    }
    m_state = EmulatorState::Running;
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::savePersistentMemory() {
    if (m_state == EmulatorState::Unloaded) {
        return Result<bool>::fail("No active save memory to flush.");
    }
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::saveState(int slot) {
    if (slot < 0 || slot > 9) {
        return Result<bool>::fail("Save state slot out of range [0-9].");
    }
    if (m_state == EmulatorState::Unloaded) {
        return Result<bool>::fail("Cannot save state when unloaded.");
    }
    return Result<bool>::ok(true);
}

Result<bool> NullEmulatorEngine::loadState(int slot) {
    if (slot < 0 || slot > 9) {
        return Result<bool>::fail("Load state slot out of range [0-9].");
    }
    if (m_state == EmulatorState::Unloaded) {
        return Result<bool>::fail("Cannot load state when unloaded.");
    }
    return Result<bool>::ok(true);
}

void NullEmulatorEngine::press(EmulatorButton /*button*/) {
    // Null implementation
}

void NullEmulatorEngine::release(EmulatorButton /*button*/) {
    // Null implementation
}

} // namespace PocketPartner::Emulator
