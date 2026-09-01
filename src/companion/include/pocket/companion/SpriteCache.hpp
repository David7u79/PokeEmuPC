#pragma once

#include "pocket/companion/SpriteKey.hpp"
#include "pocket/companion/CompositeSpriteProvider.hpp"
#include <QPixmap>
#include <map>
#include <list>
#include <string>

namespace Pocket::Companion {

class SpriteCache {
public:
    explicit SpriteCache(size_t capacity = 32);

    QPixmap get(const SpriteKey& key, int targetWidth, int targetHeight, CompositeSpriteProvider& provider);
    void clear();

    size_t size() const { return m_cacheMap.size(); }
    size_t capacity() const { return m_capacity; }
    size_t hitCount() const { return m_hitCount; }
    size_t missCount() const { return m_missCount; }

private:
    struct CacheEntry {
        std::string cacheKey;
        QPixmap pixmap;
    };

    std::string makeCacheKey(const SpriteKey& key, int width, int height) const;

    size_t m_capacity;
    std::list<CacheEntry> m_lruList;
    std::map<std::string, std::list<CacheEntry>::iterator> m_cacheMap;

    mutable size_t m_hitCount{0};
    mutable size_t m_missCount{0};
};

} // namespace Pocket::Companion
