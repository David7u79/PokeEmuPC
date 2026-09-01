#pragma once

#include "pocket/companion/CreatureSpriteProvider.hpp"
#include <map>

namespace Pocket::Companion {

class PokeSpriteProvider : public CreatureSpriteProvider {
public:
    explicit PokeSpriteProvider(const std::string& assetBaseDir = "");

    SpriteResult resolve(const SpriteKey& key) override;

    static std::string speciesIdToSlug(uint16_t speciesId);
    void setAssetBaseDir(const std::string& dir) { m_assetBaseDir = dir; }

private:
    std::string m_assetBaseDir;
    std::map<uint16_t, std::string> m_slugMap;
};

} // namespace Pocket::Companion
