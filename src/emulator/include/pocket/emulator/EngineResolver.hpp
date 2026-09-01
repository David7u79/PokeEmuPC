#pragma once

#include "pocket/core/GameSystem.hpp"
#include "pocketpartner/emulator/EmulatorEngine.hpp"
#include <memory>
#include <map>
#include <vector>

namespace Pocket::Emulator {

class EngineResolver {
public:
    EngineResolver();
    ~EngineResolver() = default;

    void registerEngine(Core::GameSystem system, std::shared_ptr<PocketPartner::Emulator::EmulatorEngine> engine);
    std::shared_ptr<PocketPartner::Emulator::EmulatorEngine> resolve(Core::GameSystem system) const;

    bool supportsSystem(Core::GameSystem system) const;
    std::vector<Core::GameSystem> supportedSystems() const;

private:
    std::map<Core::GameSystem, std::shared_ptr<PocketPartner::Emulator::EmulatorEngine>> m_engines;
};

} // namespace Pocket::Emulator
