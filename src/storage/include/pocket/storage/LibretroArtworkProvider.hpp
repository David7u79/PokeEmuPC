#pragma once

#include "pocket/storage/ArtworkProvider.hpp"
#include "pocket/storage/ArtworkCache.hpp"
#include <QObject>
#include <QNetworkAccessManager>
#include <memory>

namespace Pocket::Storage {

class LibretroArtworkProvider : public QObject, public ArtworkProvider {
    Q_OBJECT

public:
    explicit LibretroArtworkProvider(std::shared_ptr<ArtworkCache> cache, QObject* parent = nullptr);

    void fetchArtworkAsync(
        const std::string& platform,
        const std::string& canonicalTitle,
        ArtworkType type,
        std::function<void(const ArtworkResult&)> callback
    ) override;

    static std::string buildUrl(const std::string& platform, const std::string& canonicalTitle, ArtworkType type);

private:
    std::shared_ptr<ArtworkCache> m_cache;
    QNetworkAccessManager m_nam;
};

} // namespace Pocket::Storage
