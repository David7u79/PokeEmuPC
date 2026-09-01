#include "pocket/storage/ArtworkCache.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QImage>

namespace Pocket::Storage {

std::string ArtworkCache::artworkTypeToString(ArtworkType type) {
    switch (type) {
        case ArtworkType::BoxArt:       return "boxart.png";
        case ArtworkType::TitleScreen:  return "title.png";
        case ArtworkType::Screenshot:   return "screenshot.png";
        default:                        return "artwork.png";
    }
}

ArtworkCache::ArtworkCache(const std::string& cacheDir) {
    if (!cacheDir.empty()) {
        m_cacheDir = cacheDir;
    } else {
        QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_cacheDir = (appData + "/cache/artwork").toStdString();
    }
    QDir().mkpath(QString::fromStdString(m_cacheDir));
}

std::string ArtworkCache::getCachedPath(const std::string& gameId, ArtworkType type) const {
    QString path = QString("%1/%2/%3")
        .arg(QString::fromStdString(m_cacheDir))
        .arg(QString::fromStdString(gameId))
        .arg(QString::fromStdString(artworkTypeToString(type)));

    if (QFileInfo::exists(path)) {
        return path.toStdString();
    }
    return "";
}

bool ArtworkCache::saveArtwork(const std::string& gameId, ArtworkType type, const uint8_t* data, size_t size) {
    if (!data || size == 0 || size > 10 * 1024 * 1024) return false;

    // Validate QImage decode test
    QImage img;
    if (!img.loadFromData(data, static_cast<int>(size))) {
        return false;
    }

    QString dirPath = QString("%1/%2")
        .arg(QString::fromStdString(m_cacheDir))
        .arg(QString::fromStdString(gameId));
    QDir().mkpath(dirPath);

    QString filePath = QString("%1/%2")
        .arg(dirPath)
        .arg(QString::fromStdString(artworkTypeToString(type)));

    return img.save(filePath, "PNG");
}

bool ArtworkCache::isNegativeCached(const std::string& gameId, ArtworkType type) const {
    std::string key = gameId + "_" + artworkTypeToString(type);
    return m_negativeCache.find(key) != m_negativeCache.end();
}

void ArtworkCache::addNegativeCache(const std::string& gameId, ArtworkType type) {
    std::string key = gameId + "_" + artworkTypeToString(type);
    m_negativeCache.insert(key);
}

} // namespace Pocket::Storage
