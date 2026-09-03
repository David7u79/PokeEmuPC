#include "ArtworkIndex.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

namespace {

QString normalized(const QString& value)
{
    QString result = value.toLower().normalized(QString::NormalizationForm_D);
    result.remove(QRegularExpression("[\\u0300-\\u036f]"));
    result.replace(QChar(0x00f1), QChar('n'));
    result.replace(QRegularExpression("\\([^)]*\\)|\\[[^]]*\\]"), " ");
    result.replace(QRegularExpression("[^\\p{L}\\p{N}]+"), " ");
    return result.simplified();
}

int levenshtein(const QString& left, const QString& right)
{
    QVector<int> previous(right.size() + 1);
    QVector<int> current(right.size() + 1);
    for (int index = 0; index <= right.size(); ++index) {
        previous[index] = index;
    }
    for (int leftIndex = 1; leftIndex <= left.size(); ++leftIndex) {
        current[0] = leftIndex;
        for (int rightIndex = 1; rightIndex <= right.size(); ++rightIndex) {
            const int substitution = previous[rightIndex - 1]
                + (left.at(leftIndex - 1) == right.at(rightIndex - 1) ? 0 : 1);
            current[rightIndex] = std::min({previous[rightIndex] + 1, current[rightIndex - 1] + 1, substitution});
        }
        previous.swap(current);
    }
    return previous.back();
}

// An exact word is worth more than one that merely differs by a letter, otherwise
// "Pokemon Platino" scores the German "Platin-Edition" the same as the Spanish
// "Edicion Platino" and the shorter name wins on the tie-break.
double tokenScore(const QString& queryToken, const QStringList& candidateTokens)
{
    double best = 0.0;
    for (const QString& candidateToken : candidateTokens) {
        if (queryToken == candidateToken) {
            return 1.0;
        }
        if (queryToken.size() >= 4 && candidateToken.size() >= 4
            && levenshtein(queryToken, candidateToken) <= 1) {
            best = std::max(best, 0.8);
        }
    }
    return best;
}

struct MatchScore {
    QString name;
    QString normalizedName;
    double score{0.0};
};

MatchScore scoreMatch(const QString& query, const QString& candidate)
{
    const QStringList queryTokens = normalized(query).split(' ', Qt::SkipEmptyParts);
    const QString candidateNormalized = normalized(candidate);
    QStringList candidateTokens = candidateNormalized.split(' ', Qt::SkipEmptyParts);
    const QSet<QString> filler{"edicion", "edition", "version", "the", "el", "la"};
    candidateTokens.erase(std::remove_if(candidateTokens.begin(), candidateTokens.end(), [&filler](const QString& token) {
        return filler.contains(token);
    }), candidateTokens.end());
    double matched = 0.0;
    for (const QString& token : queryTokens) {
        matched += tokenScore(token, candidateTokens);
    }
    return {candidate, candidateNormalized,
            queryTokens.isEmpty() ? 0.0 : matched / queryTokens.size()};
}

} // namespace

namespace Pocket::App {

ArtworkIndex::ArtworkIndex(QString cacheDir, QObject* parent)
    : QObject(parent)
    , m_cacheDir(std::move(cacheDir))
{
}

void ArtworkIndex::ensureLoaded(const QString& repo)
{
    if (m_names.contains(repo) || m_loading.contains(repo)) {
        return;
    }
    if (loadCache(repo)) {
        emit indexLoaded(repo);
        return;
    }

    m_loading.insert(repo);
    const QUrl url(QString("https://api.github.com/repos/libretro-thumbnails/%1/git/trees/master").arg(repo));
    QNetworkReply* reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, repo] {
        const QByteArray body = reply->readAll();
        const bool succeeded = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!succeeded) {
            m_loading.remove(repo);
            emit indexFailed(repo);
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(body);
        if (!document.isObject()) {
            m_loading.remove(repo);
            emit indexFailed(repo);
            return;
        }
        const QJsonArray tree = document.object().value("tree").toArray();
        for (const QJsonValue& value : tree) {
            const QJsonObject entry = value.toObject();
            if (entry.value("path").toString() == "Named_Boxarts") {
                const QString sha = entry.value("sha").toString();
                if (!sha.isEmpty()) {
                    downloadBoxartTree(repo, sha);
                    return;
                }
            }
        }
        m_loading.remove(repo);
        emit indexFailed(repo);
    });
}

bool ArtworkIndex::isLoaded(const QString& repo) const
{
    return m_names.contains(repo);
}

QStringList ArtworkIndex::names(const QString& repo) const
{
    return m_names.value(repo);
}

void ArtworkIndex::setNames(const QString& repo, const QStringList& names)
{
    m_loading.remove(repo);
    m_names.insert(repo, names);
    saveCache(repo, names);
    emit indexLoaded(repo);
}

QString ArtworkIndex::bestMatch(const QString& query, const QStringList& names)
{
    const QStringList matches = rankedMatches(query, names, 1);
    if (matches.isEmpty() || scoreMatch(query, matches.first()).score < 0.6) {
        return {};
    }
    return matches.first();
}

QStringList ArtworkIndex::rankedMatches(const QString& query, const QStringList& names, int limit)
{
    if (limit <= 0) {
        return {};
    }
    if (normalized(query).isEmpty()) {
        QStringList result = names;
        std::sort(result.begin(), result.end(), [](const QString& left, const QString& right) {
            return QString::localeAwareCompare(left, right) < 0;
        });
        while (result.size() > limit) {
            result.removeLast();
        }
        return result;
    }
    QList<MatchScore> scored;
    scored.reserve(names.size());
    for (const QString& name : names) {
        scored.append(scoreMatch(query, name));
    }
    std::sort(scored.begin(), scored.end(), [](const MatchScore& left, const MatchScore& right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        if (left.normalizedName.size() != right.normalizedName.size()) {
            return left.normalizedName.size() < right.normalizedName.size();
        }
        return left.name < right.name;
    });
    QStringList result;
    for (const MatchScore& item : scored) {
        result.append(item.name);
        if (result.size() == limit) {
            break;
        }
    }
    return result;
}

QString ArtworkIndex::cachePath(const QString& repo) const
{
    return QDir(m_cacheDir).filePath(QString("index_%1.txt").arg(repo));
}

bool ArtworkIndex::loadCache(const QString& repo)
{
    QFile file(cachePath(repo));
    const QFileInfo info(file);
    if (!info.exists() || info.lastModified().daysTo(QDateTime::currentDateTime()) >= 30 || !file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QStringList entries;
    const QList<QByteArray> lines = file.readAll().split('\n');
    for (const QByteArray& line : lines) {
        const QString name = QString::fromUtf8(line).trimmed();
        if (!name.isEmpty()) {
            entries.append(name);
        }
    }
    m_names.insert(repo, entries);
    return true;
}

void ArtworkIndex::downloadBoxartTree(const QString& repo, const QString& sha)
{
    const QUrl url(QString("https://api.github.com/repos/libretro-thumbnails/%1/git/trees/%2").arg(repo, sha));
    QNetworkReply* reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, repo] {
        const QByteArray body = reply->readAll();
        const bool succeeded = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!succeeded) {
            m_loading.remove(repo);
            emit indexFailed(repo);
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(body);
        if (!document.isObject()) {
            m_loading.remove(repo);
            emit indexFailed(repo);
            return;
        }
        QStringList entries;
        const QJsonArray tree = document.object().value("tree").toArray();
        for (const QJsonValue& value : tree) {
            const QString path = value.toObject().value("path").toString();
            if (path.endsWith(".png", Qt::CaseInsensitive)) {
                entries.append(path.left(path.size() - 4));
            }
        }
        if (entries.isEmpty()) {
            m_loading.remove(repo);
            emit indexFailed(repo);
            return;
        }
        m_loading.remove(repo);
        m_names.insert(repo, entries);
        saveCache(repo, entries);
        emit indexLoaded(repo);
    });
}

void ArtworkIndex::saveCache(const QString& repo, const QStringList& names) const
{
    QDir().mkpath(m_cacheDir);
    QFile file(cachePath(repo));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    for (const QString& name : names) {
        file.write(name.toUtf8());
        file.write("\n");
    }
}

} // namespace Pocket::App
