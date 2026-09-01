#pragma once

#include "pocketpartner/emulator/EmulatorEngine.hpp"

namespace PocketPartner::Emulator {

class NullEmulatorEngine : public EmulatorEngine {
public:
    NullEmulatorEngine() = default;
    ~NullEmulatorEngine() override = default;

    std::vector<TargetSystem> supportedSystems() const override;

    Result<bool> loadGame(const GameLaunchRequest& request) override;

    Result<bool> start() override;
    Result<bool> pause() override;
    Result<bool> resume() override;
    Result<bool> stop() override;

    Result<bool> reset() override;

    Result<bool> savePersistentMemory() override;

    Result<bool> saveState(int slot) override;
    Result<bool> loadState(int slot) override;

    void press(EmulatorButton button) override;
    void release(EmulatorButton button) override;

    EmulatorState state() const override { return m_state; }

private:
    EmulatorState m_state{EmulatorState::Unloaded};
    GameLaunchRequest m_currentRequest;
};

} // namespace PocketPartner::Emulator
