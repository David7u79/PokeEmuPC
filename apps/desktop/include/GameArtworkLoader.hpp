#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include <memory>

namespace Pocket::Storage {
class ArtworkCache;
class LibretroArtworkProvider;
}

namespace Pocket::App {

class ArtworkIndex;

class GameArtworkLoader : public QObject {
    Q_OBJECT

public:
    explicit GameArtworkLoader(QObject* parent = nullptr);
    GameArtworkLoader(std::shared_ptr<Storage::ArtworkCache> cache, QObject* parent = nullptr);

    static QStringList titleCandidates(const QString& fileBaseName);

    void requestArtwork(const QString& gameId, const QString& title, const QString& system, const QString& romPath);
    void retryArtwork(const QString& gameId, const QString& title, const QString& system, const QString& romPath);
    void setArtworkFromFile(const QString& gameId, const QString& imagePath);

signals:
    void artworkReady(const QString& gameId, const QString& path);

private:
    struct IndexRequest {
        QString platform;
        QString repo;
        QString fileBaseName;
    };

    void requestArtworkInternal(const QString& gameId, const QString& system, const QString& romPath, bool ignoreNegativeCache);
    void fetchCandidate(const QString& gameId, const QString& platform, const QStringList& candidates, int candidateIndex);
    void fetchIndexMatch(const QString& gameId);

    std::shared_ptr<Storage::ArtworkCache> m_cache;
    Storage::LibretroArtworkProvider* m_provider{nullptr};
    ArtworkIndex* m_index{nullptr};
    QSet<QString> m_pending;
    QHash<QString, IndexRequest> m_indexRequests;
};

} // namespace Pocket::App
