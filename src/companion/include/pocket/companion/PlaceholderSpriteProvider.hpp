#pragma once

#include "pocket/companion/CreatureSpriteProvider.hpp"

namespace Pocket::Companion {

class PlaceholderSpriteProvider : public CreatureSpriteProvider {
public:
    PlaceholderSpriteProvider() = default;

    SpriteResult resolve(const SpriteKey& key) override;

    static std::string ensurePlaceholderImageExists();
};

} // namespace Pocket::Companion
