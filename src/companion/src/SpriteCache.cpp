#include "pocket/companion/SpriteCache.hpp"
#include "pocket/companion/PlaceholderSpriteProvider.hpp"
#include <QImage>
#include <QFileInfo>

namespace Pocket::Companion {

SpriteCache::SpriteCache(size_t capacity)
    : m_capacity(capacity) {}

std::string SpriteCache::makeCacheKey(const SpriteKey& key, int width, int height) const {
    return key.toString() + "_W:" + std::to_string(width) + "_H:" + std::to_string(height);
}

void SpriteCache::clear() {
    m_lruList.clear();
    m_cacheMap.clear();
}

QPixmap SpriteCache::get(const SpriteKey& key, int targetWidth, int targetHeight, CompositeSpriteProvider& provider) {
    std::string ck = makeCacheKey(key, targetWidth, targetHeight);

    auto it = m_cacheMap.find(ck);
    if (it != m_cacheMap.end()) {
        m_hitCount++;
        // Move entry to front of LRU list
        m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
        return it->second->pixmap;
    }

    m_missCount++;

    // Resolve sprite asset path from provider
    SpriteResult res = provider.resolve(key);
    QImage rawImg;

    if (res.success && QFileInfo::exists(QString::fromStdString(res.imagePath))) {
        rawImg.load(QString::fromStdString(res.imagePath));
    }

    if (rawImg.isNull()) {
        // Fallback to emergency placeholder
        std::string placeholderPath = PlaceholderSpriteProvider::ensurePlaceholderImageExists();
        rawImg.load(QString::fromStdString(placeholderPath));
    }

    // Scale using Nearest-Neighbor interpolation (Qt::FastTransformation) to preserve crisp pixel-art!
    QPixmap scaledPixmap;
    if (targetWidth > 0 && targetHeight > 0) {
        QImage scaled = rawImg.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
        scaledPixmap = QPixmap::fromImage(scaled);
    } else {
        scaledPixmap = QPixmap::fromImage(rawImg);
    }

    // Evict oldest item if capacity reached
    if (m_lruList.size() >= m_capacity) {
        auto oldest = m_lruList.back();
        m_cacheMap.erase(oldest.cacheKey);
        m_lruList.pop_back();
    }

    // Insert new entry at front
    m_lruList.push_front({ck, scaledPixmap});
    m_cacheMap[ck] = m_lruList.begin();

    return scaledPixmap;
}

} // namespace Pocket::Companion
