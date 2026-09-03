#pragma once

#include <QObject>
#include <QHash>
#include <QNetworkAccessManager>
#include <QSet>
#include <QStringList>

namespace Pocket::App {

class ArtworkIndex : public QObject {
    Q_OBJECT

public:
    explicit ArtworkIndex(QString cacheDir, QObject* parent = nullptr);

    void ensureLoaded(const QString& repo);
    bool isLoaded(const QString& repo) const;
    QStringList names(const QString& repo) const;
    void setNames(const QString& repo, const QStringList& names);

    static QString bestMatch(const QString& query, const QStringList& names);

signals:
    void indexLoaded(const QString& repo);

private:
    QString cachePath(const QString& repo) const;
    bool loadCache(const QString& repo);
    void downloadBoxartTree(const QString& repo, const QString& sha);
    void saveCache(const QString& repo, const QStringList& names) const;

    QString m_cacheDir;
    QNetworkAccessManager m_network;
    QHash<QString, QStringList> m_names;
    QSet<QString> m_loading;
};

} // namespace Pocket::App
