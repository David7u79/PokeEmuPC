#pragma once

#include "pocket/core/GameSystem.hpp"
#include <memory>

namespace Pocket::Emulator {
class EmulatorEngine;

class EngineResolver {
public:
    void setCorePath(Core::GameSystem system, const std::string& path);
    std::shared_ptr<EmulatorEngine> createFor(Core::GameSystem system) const;
    bool supports(Core::GameSystem system) const;

private:
    std::string m_mgbaCorePath;
    std::string m_melonDsCorePath;
};

} // namespace Pocket::Emulator
