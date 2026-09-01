#include "pocket/storage/LibretroArtworkProvider.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace Pocket::Storage {

LibretroArtworkProvider::LibretroArtworkProvider(std::shared_ptr<ArtworkCache> cache, QObject* parent)
    : QObject(parent)
    , m_cache(cache) {
}

std::string LibretroArtworkProvider::buildUrl(const std::string& platform, const std::string& canonicalTitle, ArtworkType type) {
    std::string platFolder;
    if (platform.find("Advance") != std::string::npos) platFolder = "Nintendo_-_Game_Boy_Advance";
    else if (platform.find("Color") != std::string::npos) platFolder = "Nintendo_-_Game_Boy_Color";
    else if (platform.find("DS") != std::string::npos) platFolder = "Nintendo_-_Nintendo_DS";
    else platFolder = "Nintendo_-_Game_Boy";

    std::string catFolder;
    switch (type) {
        case ArtworkType::BoxArt:       catFolder = "Named_Boxarts"; break;
        case ArtworkType::TitleScreen:  catFolder = "Named_Titles"; break;
        case ArtworkType::Screenshot:   catFolder = "Named_Snaps"; break;
    }

    // Escape title for URL encoding
    QString titleEscaped = QString::fromStdString(canonicalTitle);
    titleEscaped.replace("&", "%26");

    return QString("https://raw.githubusercontent.com/libretro-thumbnails/%1/master/%2/%3.png")
        .arg(QString::fromStdString(platFolder))
        .arg(QString::fromStdString(catFolder))
        .arg(titleEscaped)
        .toStdString();
}

void LibretroArtworkProvider::fetchArtworkAsync(
    const std::string& platform,
    const std::string& canonicalTitle,
    ArtworkType type,
    std::function<void(const ArtworkResult&)> callback
) {
    std::string gameId = canonicalTitle;

    // Check negative cache first
    if (m_cache->isNegativeCached(gameId, type)) {
        ArtworkResult res;
        res.success = false;
        res.errorMessage = "Negative cache hit - artwork previously unavailable.";
        if (callback) callback(res);
        return;
    }

    // Check disk cache hit
    std::string cached = m_cache->getCachedPath(gameId, type);
    if (!cached.empty()) {
        ArtworkResult res;
        res.success = true;
        res.cachedFilePath = cached;
        if (callback) callback(res);
        return;
    }

    std::string urlStr = buildUrl(platform, canonicalTitle, type);
    QNetworkRequest request(QUrl(QString::fromStdString(urlStr)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, type, callback]() {
        reply->deleteLater();
        ArtworkResult result;

        if (reply->error() != QNetworkReply::NoError) {
            result.success = false;
            result.errorMessage = reply->errorString().toStdString();
            m_cache->addNegativeCache(gameId, type);
            if (callback) callback(result);
            return;
        }

        QByteArray data = reply->readAll();
        if (data.size() == 0 || data.size() > 10 * 1024 * 1024) {
            result.success = false;
            result.errorMessage = "Invalid payload size";
            m_cache->addNegativeCache(gameId, type);
            if (callback) callback(result);
            return;
        }

        if (m_cache->saveArtwork(gameId, type, reinterpret_cast<const uint8_t*>(data.constData()), data.size())) {
            result.success = true;
            result.cachedFilePath = m_cache->getCachedPath(gameId, type);
        } else {
            result.success = false;
            result.errorMessage = "Failed to decode/save artwork image";
            m_cache->addNegativeCache(gameId, type);
        }

        if (callback) callback(result);
    });
}

} // namespace Pocket::Storage
