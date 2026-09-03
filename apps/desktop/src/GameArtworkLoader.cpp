#include "GameArtworkLoader.hpp"
#include "pocket/storage/ArtworkCache.hpp"
#include "pocket/storage/LibretroArtworkProvider.hpp"
#include "pocket/storage/GameMetadataResolver.hpp"
#include <QFileInfo>
namespace Pocket::App {
GameArtworkLoader::GameArtworkLoader(QObject* parent) : GameArtworkLoader(std::make_shared<Storage::ArtworkCache>(), parent) {}
GameArtworkLoader::GameArtworkLoader(std::shared_ptr<Storage::ArtworkCache> cache, QObject* parent) : QObject(parent), m_cache(std::move(cache)) { m_provider = new Storage::LibretroArtworkProvider(m_cache, this); }
void GameArtworkLoader::requestArtwork(const QString& gameId, const QString&, const QString& system, const QString& romPath) {
 if (gameId.isEmpty() || m_pending.contains(gameId)) return; const auto cached=m_cache->getCachedPath(gameId.toStdString(), Storage::ArtworkType::BoxArt); if (!cached.empty()) { emit artworkReady(gameId, QString::fromStdString(cached)); return; } if (m_cache->isNegativeCached(gameId.toStdString(), Storage::ArtworkType::BoxArt)) return;
 const QString platform=system=="GB"?"Nintendo - Game Boy":system=="GBC"?"Nintendo - Game Boy Color":system=="GBA"?"Nintendo - Game Boy Advance":system=="NDS"?"Nintendo - Nintendo DS":QString(); if(platform.isEmpty()) return; m_pending.insert(gameId); const std::string canonical=Storage::GameMetadataResolver::normalizeFilename(QFileInfo(romPath).completeBaseName().toStdString());
 m_provider->fetchArtworkAsync(platform.toStdString(), canonical, Storage::ArtworkType::BoxArt, [this, gameId](const Storage::ArtworkResult& result) { QMetaObject::invokeMethod(this, [this, gameId, result] { m_pending.remove(gameId); if(result.success) emit artworkReady(gameId, QString::fromStdString(result.cachedFilePath)); }, Qt::QueuedConnection); });
}
}
