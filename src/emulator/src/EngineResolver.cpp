#include "pocket/emulator/EngineResolver.hpp"
#include "pocketpartner/emulator/NullEmulatorEngine.hpp"

namespace Pocket::Emulator {

EngineResolver::EngineResolver() {
    // Default fallback mock engine for initial foundation phase
    auto nullEngine = std::make_shared<PocketPartner::Emulator::NullEmulatorEngine>();
    registerEngine(Core::GameSystem::GB, nullEngine);
    registerEngine(Core::GameSystem::GBC, nullEngine);
    registerEngine(Core::GameSystem::GBA, nullEngine);
    registerEngine(Core::GameSystem::NDS, nullEngine);
}

void EngineResolver::registerEngine(Core::GameSystem system,
                                     std::shared_ptr<PocketPartner::Emulator::EmulatorEngine> engine) {
    m_engines[system] = std::move(engine);
}

std::shared_ptr<PocketPartner::Emulator::EmulatorEngine> EngineResolver::resolve(Core::GameSystem system) const {
    auto it = m_engines.find(system);
    if (it != m_engines.end()) {
        return it->second;
    }
    return nullptr;
}

bool EngineResolver::supportsSystem(Core::GameSystem system) const {
    return m_engines.find(system) != m_engines.end();
}

std::vector<Core::GameSystem> EngineResolver::supportedSystems() const {
    std::vector<Core::GameSystem> list;
    for (const auto& [sys, engine] : m_engines) {
        list.push_back(sys);
    }
    return list;
}

} // namespace Pocket::Emulator
