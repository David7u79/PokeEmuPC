#pragma once

#include "pocket/storage/ArtworkProvider.hpp"
#include <string>
#include <vector>
#include <set>

namespace Pocket::Storage {

class ArtworkCache {
public:
    explicit ArtworkCache(const std::string& cacheDir = "");

    std::string getCachedPath(const std::string& gameId, ArtworkType type) const;
    bool saveArtwork(const std::string& gameId, ArtworkType type, const uint8_t* data, size_t size);

    bool isNegativeCached(const std::string& gameId, ArtworkType type) const;
    void addNegativeCache(const std::string& gameId, ArtworkType type);

    std::string cacheDir() const { return m_cacheDir; }

    static std::string artworkTypeToString(ArtworkType type);

private:
    std::string m_cacheDir;
    mutable std::set<std::string> m_negativeCache;
};

} // namespace Pocket::Storage
