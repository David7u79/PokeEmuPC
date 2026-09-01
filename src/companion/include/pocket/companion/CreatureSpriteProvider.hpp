#pragma once

#include "pocket/companion/SpriteKey.hpp"
#include <string>

namespace Pocket::Companion {

struct SpriteResult {
    bool success{false};
    std::string imagePath;
    std::string providerName;
    std::string errorMessage;
    bool isShiny{false};
    bool isFallback{false};
};

class CreatureSpriteProvider {
public:
    virtual ~CreatureSpriteProvider() = default;
    virtual SpriteResult resolve(const SpriteKey& key) = 0;
};

} // namespace Pocket::Companion
