#pragma once

#include <string>

namespace Pocket::Core {

enum class GameSource {
    INTERNAL_EMULATOR,
    EXTERNAL_SAVE
};

class GameSourceUtils {
public:
    static std::string toString(GameSource source) {
        switch (source) {
            case GameSource::INTERNAL_EMULATOR: return "INTERNAL_EMULATOR";
            case GameSource::EXTERNAL_SAVE:    return "EXTERNAL_SAVE";
        }
        return "INTERNAL_EMULATOR";
    }

    static GameSource fromString(const std::string& str) {
        if (str == "EXTERNAL_SAVE") return GameSource::EXTERNAL_SAVE;
        return GameSource::INTERNAL_EMULATOR;
    }
};

} // namespace Pocket::Core
