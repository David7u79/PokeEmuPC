#include "GameArtworkLoader.hpp"
#include "ArtworkIndex.hpp"

#include "pocket/storage/ArtworkCache.hpp"
#include "pocket/storage/GameMetadataResolver.hpp"
#include "pocket/storage/LibretroArtworkProvider.hpp"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QRegularExpression>

namespace {

QString platformForSystem(const QString& system)
{
    if (system == "GB") return "Nintendo - Game Boy";
    if (system == "GBC") return "Nintendo - Game Boy Color";
    if (system == "GBA") return "Nintendo - Game Boy Advance";
    if (system == "NDS") return "Nintendo - Nintendo DS";
    return {};
}

QString repoForSystem(const QString& system)
{
    if (system == "GB") return "Nintendo_-_Game_Boy";
    if (system == "GBC") return "Nintendo_-_Game_Boy_Color";
    if (system == "GBA") return "Nintendo_-_Game_Boy_Advance";
    if (system == "NDS") return "Nintendo_-_Nintendo_DS";
    return {};
}

void appendCandidate(QStringList& candidates, const QString& candidate)
{
    const QString trimmed = candidate.trimmed();
    if (!trimmed.isEmpty() && !candidates.contains(trimmed)) candidates.append(trimmed);
}

} // namespace

namespace Pocket::App {

GameArtworkLoader::GameArtworkLoader(QObject* parent)
    : GameArtworkLoader(std::make_shared<Storage::ArtworkCache>(), parent)
{
}

GameArtworkLoader::GameArtworkLoader(std::shared_ptr<Storage::ArtworkCache> cache, QObject* parent)
    : QObject(parent)
    , m_cache(std::move(cache))
    , m_provider(new Storage::LibretroArtworkProvider(m_cache, this))
    , m_index(new ArtworkIndex(QString::fromStdString(m_cache->cacheDir()), this))
{
    connect(m_index, &ArtworkIndex::indexLoaded, this, [this](const QString& repo) {
        const QList<QString> gameIds = m_indexRequests.keys();
        for (const QString& gameId : gameIds) {
            if (m_indexRequests.value(gameId).repo == repo) {
                fetchIndexMatch(gameId);
            }
        }
    });
}

QStringList GameArtworkLoader::titleCandidates(const QString& fileBaseName)
{
    const QString rawTitle = QFileInfo(fileBaseName).completeBaseName();
    const QString normalized = QString::fromStdString(Storage::GameMetadataResolver::normalizeFilename(rawTitle.toStdString()));
    const QString withoutTags = QString(rawTitle)
                                    .remove(QRegularExpression("\\s*\\([^)]*\\)"))
                                    .remove(QRegularExpression("\\s*\\[[^]]*\\]"))
                                    .replace(QRegularExpression("\\s{2,}"), " ")
                                    .trimmed();
    QStringList candidates;
    appendCandidate(candidates, normalized);
    appendCandidate(candidates, rawTitle);
    appendCandidate(candidates, withoutTags);
    for (const QString& region : {" (USA)", " (USA, Europe)", " (Europe)", " (Japan)"}) {
        appendCandidate(candidates, withoutTags + region);
    }
    const QRegularExpression article("^(The|An|A)\\s+(.+)$", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = article.match(withoutTags);
    if (match.hasMatch()) appendCandidate(candidates, QString("%1, %2").arg(match.captured(2), match.captured(1)));
    return candidates;
}

void GameArtworkLoader::requestArtwork(const QString& gameId, const QString&, const QString& system, const QString& romPath)
{
    requestArtworkInternal(gameId, system, romPath, false);
}

void GameArtworkLoader::retryArtwork(const QString& gameId, const QString&, const QString& system, const QString& romPath)
{
    requestArtworkInternal(gameId, system, romPath, true);
}

void GameArtworkLoader::requestArtworkInternal(const QString& gameId, const QString& system, const QString& romPath, bool ignoreNegativeCache)
{
    if (gameId.isEmpty() || m_pending.contains(gameId)) return;
    const auto cached = m_cache->getCachedPath(gameId.toStdString(), Storage::ArtworkType::BoxArt);
    if (!cached.empty()) {
        emit artworkReady(gameId, QString::fromStdString(cached));
        return;
    }
    if (!ignoreNegativeCache && m_cache->isNegativeCached(gameId.toStdString(), Storage::ArtworkType::BoxArt)) return;
    const QString platform = platformForSystem(system);
    const QString repo = repoForSystem(system);
    if (platform.isEmpty() || repo.isEmpty()) return;
    m_pending.insert(gameId);
    m_indexRequests.insert(gameId, {platform, repo, QFileInfo(romPath).completeBaseName()});
    fetchCandidate(gameId, platform, titleCandidates(QFileInfo(romPath).fileName()), 0);
}

void GameArtworkLoader::fetchCandidate(const QString& gameId, const QString& platform, const QStringList& candidates, int candidateIndex)
{
    if (candidateIndex >= candidates.size()) {
        fetchIndexMatch(gameId);
        return;
    }
    m_provider->fetchArtworkAsync(platform.toStdString(), candidates.at(candidateIndex).toStdString(), Storage::ArtworkType::BoxArt,
        [this, gameId, platform, candidates, candidateIndex](const Storage::ArtworkResult& result) {
            QMetaObject::invokeMethod(this, [this, gameId, platform, candidates, candidateIndex, result] {
                if (result.success) {
                    m_pending.remove(gameId);
                    m_indexRequests.remove(gameId);
                    adoptFetchedFile(gameId, QString::fromStdString(result.cachedFilePath));
                    return;
                }
                fetchCandidate(gameId, platform, candidates, candidateIndex + 1);
            }, Qt::QueuedConnection);
        });
}

void GameArtworkLoader::adoptFetchedFile(const QString& gameId, const QString& fetchedPath)
{
    // The provider files a download under the libretro title it asked for, so the
    // next launch looks for it under the game's id and downloads it all over again.
    // Store a copy under the id, which is the key everything else uses.
    QFile file(fetchedPath);
    QByteArray bytes;
    if (file.open(QIODevice::ReadOnly)) bytes = file.readAll();
    if (!bytes.isEmpty()
        && m_cache->saveArtwork(gameId.toStdString(), Storage::ArtworkType::BoxArt,
                                reinterpret_cast<const uint8_t*>(bytes.constData()), size_t(bytes.size()))) {
        const auto cached = m_cache->getCachedPath(gameId.toStdString(), Storage::ArtworkType::BoxArt);
        if (!cached.empty()) {
            emit artworkReady(gameId, QString::fromStdString(cached));
            return;
        }
    }
    emit artworkReady(gameId, fetchedPath);
}

void GameArtworkLoader::fetchIndexMatch(const QString& gameId)
{
    const IndexRequest request = m_indexRequests.value(gameId);
    if (request.repo.isEmpty()) {
        m_pending.remove(gameId);
        return;
    }
    if (!m_index->isLoaded(request.repo)) {
        m_index->ensureLoaded(request.repo);
        return;
    }
    const QString match = ArtworkIndex::bestMatch(request.fileBaseName, m_index->names(request.repo));
    m_indexRequests.remove(gameId);
    if (match.isEmpty()) {
        m_pending.remove(gameId);
        return;
    }
    m_provider->fetchArtworkAsync(request.platform.toStdString(), match.toStdString(), Storage::ArtworkType::BoxArt,
        [this, gameId](const Storage::ArtworkResult& result) {
            QMetaObject::invokeMethod(this, [this, gameId, result] {
                m_pending.remove(gameId);
                if (result.success) {
                    adoptFetchedFile(gameId, QString::fromStdString(result.cachedFilePath));
                }
            }, Qt::QueuedConnection);
        });
}

void GameArtworkLoader::setArtworkFromFile(const QString& gameId, const QString& imagePath)
{
    if (gameId.isEmpty()) return;
    QImage image(imagePath);
    QFile file(imagePath);
    if (image.isNull() || !file.open(QIODevice::ReadOnly)) return;
    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) return;
    if (!m_cache->saveArtwork(gameId.toStdString(), Storage::ArtworkType::BoxArt,
            reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size()))) return;
    const auto cached = m_cache->getCachedPath(gameId.toStdString(), Storage::ArtworkType::BoxArt);
    if (!cached.empty()) emit artworkReady(gameId, QString::fromStdString(cached));
}

void GameArtworkLoader::useIndexName(const QString& gameId, const QString& system, const QString& indexName)
{
    const QString platform = platformForSystem(system);
    if (gameId.isEmpty() || indexName.isEmpty() || platform.isEmpty()) {
        return;
    }
    m_provider->fetchArtworkAsync(platform.toStdString(), indexName.toStdString(), Storage::ArtworkType::BoxArt,
        [this, gameId](const Storage::ArtworkResult& result) {
            QMetaObject::invokeMethod(this, [this, gameId, result] {
                if (!result.success) {
                    return;
                }
                QFile file(QString::fromStdString(result.cachedFilePath));
                if (!file.open(QIODevice::ReadOnly)) {
                    return;
                }
                const QByteArray bytes = file.readAll();
                if (bytes.isEmpty() || !m_cache->saveArtwork(gameId.toStdString(), Storage::ArtworkType::BoxArt,
                        reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size()))) {
                    return;
                }
                const auto cached = m_cache->getCachedPath(gameId.toStdString(), Storage::ArtworkType::BoxArt);
                if (!cached.empty()) {
                    emit artworkReady(gameId, QString::fromStdString(cached));
                }
            }, Qt::QueuedConnection);
        });
}

ArtworkIndex* GameArtworkLoader::index() const
{
    return m_index;
}

} // namespace Pocket::App
