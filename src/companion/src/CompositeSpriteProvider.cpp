#include "pocket/companion/CompositeSpriteProvider.hpp"

namespace Pocket::Companion {

CompositeSpriteProvider::CompositeSpriteProvider() = default;

void CompositeSpriteProvider::addProvider(std::shared_ptr<CreatureSpriteProvider> provider) {
    if (provider) {
        m_providers.push_back(provider);
    }
}

SpriteResult CompositeSpriteProvider::resolve(const SpriteKey& key) {
    for (const auto& provider : m_providers) {
        SpriteResult result = provider->resolve(key);
        if (result.success) {
            return result;
        }
    }

    // Default emergency fallback result
    SpriteResult emergency;
    emergency.success = false;
    emergency.providerName = "None";
    emergency.errorMessage = "No provider succeeded in resolving sprite";
    return emergency;
}

} // namespace Pocket::Companion
