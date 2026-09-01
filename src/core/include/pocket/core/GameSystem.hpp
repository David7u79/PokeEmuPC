#pragma once

#include <string>
#include <optional>

namespace Pocket::Core {

enum class GameSystem {
    Unknown,
    GB,
    GBC,
    GBA,
    NDS
};

class GameSystemUtils {
public:
    static std::string toString(GameSystem system);
    static GameSystem fromString(const std::string& str);

    static std::optional<GameSystem> detectFromExtension(const std::string& filePathOrExt);
    static std::string defaultExtension(GameSystem system);
};

} // namespace Pocket::Core
