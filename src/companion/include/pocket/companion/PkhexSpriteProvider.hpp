#pragma once

#include "pocket/companion/CreatureSpriteProvider.hpp"

namespace Pocket::Companion {

class PkhexSpriteProvider : public CreatureSpriteProvider {
public:
    explicit PkhexSpriteProvider(const std::string& assetBaseDir = "");

    SpriteResult resolve(const SpriteKey& key) override;

    void setAssetBaseDir(const std::string& dir) { m_assetBaseDir = dir; }

private:
    std::string m_assetBaseDir;
};

} // namespace Pocket::Companion
