#include "pocket/emulator/EngineResolver.hpp"
#include "pocket/emulator/MelonDsEngine.hpp"
#include "pocket/emulator/MgbaEngine.hpp"

namespace Pocket::Emulator {

void EngineResolver::setCorePath(Core::GameSystem system, const std::string& path) {
    switch (system) {
    case Core::GameSystem::GB:
    case Core::GameSystem::GBC:
    case Core::GameSystem::GBA:
        m_mgbaCorePath = path;
        break;
    case Core::GameSystem::NDS:
        m_melonDsCorePath = path;
        break;
    default:
        break;
    }
}

std::shared_ptr<EmulatorEngine> EngineResolver::createFor(Core::GameSystem system) const {
    switch (system) {
    case Core::GameSystem::GB:
    case Core::GameSystem::GBC:
    case Core::GameSystem::GBA:
        return std::make_shared<MgbaEngine>(m_mgbaCorePath);
    case Core::GameSystem::NDS:
        return std::make_shared<MelonDsEngine>(m_melonDsCorePath);
    default:
        return nullptr;
    }
}

bool EngineResolver::supports(Core::GameSystem system) const {
    return system == Core::GameSystem::GB || system == Core::GameSystem::GBC || system == Core::GameSystem::GBA ||
           system == Core::GameSystem::NDS;
}

} // namespace Pocket::Emulator
