#pragma once
#include <QObject>
#include <QString>
#include <QSet>
#include <memory>
namespace Pocket::Storage { class ArtworkCache; class LibretroArtworkProvider; }
namespace Pocket::App {
class GameArtworkLoader : public QObject {
    Q_OBJECT
public:
    explicit GameArtworkLoader(QObject* parent = nullptr);
    GameArtworkLoader(std::shared_ptr<Storage::ArtworkCache> cache, QObject* parent = nullptr);
    void requestArtwork(const QString& gameId, const QString& title, const QString& system, const QString& romPath);
signals: void artworkReady(const QString& gameId, const QString& path);
private:
    std::shared_ptr<Storage::ArtworkCache> m_cache;
    Storage::LibretroArtworkProvider *m_provider{nullptr}; QSet<QString> m_pending;
}; }
