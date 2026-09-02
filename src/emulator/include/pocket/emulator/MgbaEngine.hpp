#pragma once

#include "pocket/emulator/LibretroEngineBase.hpp"

namespace Pocket::Emulator {
class MgbaEngine final : public LibretroEngineBase {
public:
    explicit MgbaEngine(const std::string& coreLibraryPath = "") : LibretroEngineBase(coreLibraryPath) {}
    ~MgbaEngine() override = default;
};
} // namespace Pocket::Emulator
