#pragma once

#include "pocketpartner/emulator/EmulatorTypes.hpp"
#include <vector>

namespace PocketPartner::Emulator {

class EmulatorEngine {
public:
    virtual ~EmulatorEngine() = default;

    virtual std::vector<TargetSystem> supportedSystems() const = 0;

    virtual Result<bool> loadGame(const GameLaunchRequest& request) = 0;

    virtual Result<bool> start() = 0;
    virtual Result<bool> pause() = 0;
    virtual Result<bool> resume() = 0;
    virtual Result<bool> stop() = 0;

    virtual Result<bool> reset() = 0;

    virtual Result<bool> savePersistentMemory() = 0;

    virtual Result<bool> saveState(int slot) = 0;
    virtual Result<bool> loadState(int slot) = 0;

    virtual void press(EmulatorButton button) = 0;
    virtual void release(EmulatorButton button) = 0;

    virtual EmulatorState state() const = 0;
};

} // namespace PocketPartner::Emulator
