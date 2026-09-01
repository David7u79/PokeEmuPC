#pragma once

#include "pocket/companion/CreatureSpriteProvider.hpp"
#include <vector>
#include <memory>

namespace Pocket::Companion {

class CompositeSpriteProvider : public CreatureSpriteProvider {
public:
    CompositeSpriteProvider();

    void addProvider(std::shared_ptr<CreatureSpriteProvider> provider);
    SpriteResult resolve(const SpriteKey& key) override;

    const std::vector<std::shared_ptr<CreatureSpriteProvider>>& providers() const { return m_providers; }

private:
    std::vector<std::shared_ptr<CreatureSpriteProvider>> m_providers;
};

} // namespace Pocket::Companion
