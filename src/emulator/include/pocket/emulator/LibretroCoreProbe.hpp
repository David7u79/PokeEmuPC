#pragma once

#include "pocket/core/GameSystem.hpp"
#include <string>

namespace Pocket::Emulator {

struct LibretroCoreDescription {
    bool valid{false};
    std::string libraryName;
    std::string libraryVersion;
    std::string validExtensions;
    bool needFullpath{false};
    bool blockExtract{false};
    unsigned apiVersion{0};
    std::string error;
};

// Loads a core only long enough to read its metadata. It never initializes the core.
LibretroCoreDescription probeLibretroCore(const std::string& path);

bool coreSupportsSystem(const LibretroCoreDescription& core, Core::GameSystem system);

} // namespace Pocket::Emulator
