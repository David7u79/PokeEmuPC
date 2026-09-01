#pragma once

#include "pocket/core/GameId.hpp"
#include "pocket/core/GameSystem.hpp"
#include "pocket/core/GameSource.hpp"
#include <string>
#include <cstdint>

namespace Pocket::Core {

struct Game {
    GameId id;
    std::string title;
    GameSystem system{GameSystem::Unknown};
    std::string romPath;
    std::string sha256;
    uint64_t fileSizeBytes{0};
    GameSource source{GameSource::INTERNAL_EMULATOR};
    int64_t importedAtTs{0};

    bool isValid() const {
        return !id.isEmpty() && !title.empty() && system != GameSystem::Unknown && !romPath.empty();
    }
};

} // namespace Pocket::Core
