#include "GameCardDelegate.hpp"

#include "Icons.hpp"
#include "Theme.hpp"

#include <QHash>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPixmapCache>

#include <cmath>

namespace {

constexpr int SystemRole = Qt::UserRole + 2;
constexpr int ArtworkRole = Qt::UserRole + 3;

// The stage only exists to align cards. It is never painted: what the eye sees is
// the box art and its shadow, sitting on the application background.
// Pokémon box art runs from roughly square to mildly portrait. Clamping keeps one
// odd scan from stretching every row.
constexpr double MinCoverAspect = 0.90;
constexpr double MaxCoverAspect = 1.45;
// How much of the stage the artwork fills. The rest is the gap between covers.
constexpr double CoverFill = 0.94;
// Title plus platform. Constant, so every title in a row shares a baseline.
constexpr int MetadataHeight = 40;

QString initials(const QString& title)
{
    QString result;
    for (const QString& word : title.split(' ', Qt::SkipEmptyParts)) {
        result += word.left(1).toUpper();
        if (result.size() == 2) break;
    }
    return result.isEmpty() ? QStringLiteral("?") : result;
}

QColor systemColor(const QString& system)
{
    if (system == "GB") return QColor("#6b7a5a");
    if (system == "GBC") return QColor("#5b4b8a");
    if (system == "GBA") return QColor("#3c438f");
    return QColor("#4e5964");
}

QString systemName(const QString& system)
{
    if (system == "GB") return QStringLiteral("Game Boy");
    if (system == "GBC") return QStringLiteral("Game Boy Color");
    if (system == "GBA") return QStringLiteral("Game Boy Advance");
    if (system == "NDS") return QStringLiteral("Nintendo DS");
    return system;
}

QColor rgba(int r, int g, int b, double alpha)
{
    QColor color(r, g, b);
    color.setAlphaF(alpha);
    return color;
}

// Header read, cached: the cell needs the artwork's proportions before deciding
// how big to draw it.
QSize coverSourceSize(const QString& path)
{
    if (path.isEmpty()) return {};
    static QHash<QString, QSize> sizes;
    const auto known = sizes.constFind(path);
    if (known != sizes.constEnd()) return known.value();
    const QSize size = QImageReader(path).size();
    sizes.insert(path, size);
    return size;
}

// Artwork lives on disk and was being reloaded on every paint. Cached by path,
// then again by target size so the smooth scale only runs when zoom changes.
QPixmap coverPixmap(const QString& path, const QSize& bounds)
{
    if (path.isEmpty() || bounds.isEmpty()) return {};
    const QString scaledKey = QStringLiteral("pp:cover:%1@%2x%3").arg(path).arg(bounds.width()).arg(bounds.height());
    // Fit inside the requested box; the box already carries the normalised area.
    QPixmap scaled;
    if (QPixmapCache::find(scaledKey, &scaled)) return scaled;

    const QString sourceKey = QStringLiteral("pp:cover:src:") + path;
    QPixmap source;
    if (!QPixmapCache::find(sourceKey, &source)) {
        source = QPixmap(path);
        if (source.isNull()) return {};
        QPixmapCache::insert(sourceKey, source);
    }
    // contain, never crop: the whole box is what the player recognises.
    scaled = source.scaled(bounds, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmapCache::insert(scaledKey, scaled);
    return scaled;
}

} // namespace

namespace Pocket::App {

GameCardDelegate::GameCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void GameCardDelegate::setCardWidth(int width)
{
    m_cardWidth = width;
}

void GameCardDelegate::setCoverAspect(double aspect)
{
    m_coverAspect = qBound(MinCoverAspect, aspect, MaxCoverAspect);
}

int GameCardDelegate::stageWidth() const
{
    return m_cardWidth - 6;
}

// Every cover is drawn to the same area, so a squarer GBA box and a taller DS box
// carry the same visual weight instead of one looking a third bigger.
QSizeF GameCardDelegate::artSize(double aspect) const
{
    const double side = stageWidth() * CoverFill;
    const double ratio = qBound(MinCoverAspect, aspect, MaxCoverAspect);
    QSizeF size(side / std::sqrt(ratio), side * std::sqrt(ratio));
    if (size.width() > stageWidth()) size *= stageWidth() / size.width();
    if (size.height() > stageHeight() - 4) size *= (stageHeight() - 4) / size.height();
    return size;
}

int GameCardDelegate::stageHeight() const
{
    // Exactly what the tallest cover in the library needs at that area, plus room
    // for its shadow. No portrait guess, so no dead space in any row.
    const double ratio = qBound(MinCoverAspect, m_coverAspect, MaxCoverAspect);
    return qRound(stageWidth() * CoverFill * std::sqrt(ratio)) + 6;
}

int GameCardDelegate::metadataHeight() const
{
    return MetadataHeight;
}

QSize GameCardDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return {m_cardWidth, stageHeight() + metadataHeight()};
}

void GameCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QString title = index.data().toString();
    const QString system = index.data(SystemRole).toString();
    const QString artworkPath = index.data(ArtworkRole).toString();

    const QRect stage(option.rect.left(), option.rect.top(), option.rect.width(), stageHeight());
    const QRect metadata(option.rect.left(), stage.bottom() + 1, option.rect.width(), metadataHeight());

    // Bottom aligned inside the stage: boxes stand on a common shelf line and the
    // title sits right under the artwork whatever the platform's proportions are.
    const QRect bounds = stage.adjusted(3, 3, -3, -3);
    const QSize source = coverSourceSize(artworkPath);
    const double ratio = source.isValid() && source.width() > 0
        ? double(source.height()) / source.width() : 1.0;
    const QSizeF target = artSize(ratio);
    const QPixmap pixmap = coverPixmap(artworkPath, target.toSize());

    QRect art;
    if (!pixmap.isNull()) {
        art = QRect(QPoint(0, 0), pixmap.size());
    } else {
        art = QRect(QPoint(0, 0), artSize(1.35).toSize());
    }
    const int lift = hovered ? -3 : (selected ? -2 : 0);
    art.moveCenter(QPoint(bounds.center().x(), bounds.center().y()));
    art.moveBottom(bounds.bottom() + lift);

    painter->setPen(Qt::NoPen);
    const int shadowDepth = hovered ? 4 : 3;
    for (int step = shadowDepth; step >= 1; --step) {
        painter->setBrush(rgba(0, 0, 0, hovered ? 0.13 : 0.10));
        painter->drawRoundedRect(art.adjusted(-step, step, step, step + 3), 4, 4);
    }

    if (!pixmap.isNull()) {
        painter->drawPixmap(art.topLeft(), pixmap);
    } else {
        QLinearGradient gradient(art.topLeft(), art.bottomRight());
        const QColor color = systemColor(system);
        gradient.setColorAt(0, color.darker(130));
        gradient.setColorAt(1, color.darker(175));
        painter->setBrush(gradient);
        painter->drawRoundedRect(art, 3, 3);

        QFont mark = option.font;
        mark.setPixelSize(qMax(7, art.width() / 17));
        mark.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        painter->setFont(mark);
        painter->setPen(rgba(255, 255, 255, 0.30));
        painter->drawText(art.adjusted(0, 14, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "POCKETPARTNER");

        QFont font = option.font;
        font.setPixelSize(qMax(18, art.width() / 3));
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(rgba(255, 255, 255, 0.58));
        painter->drawText(art, Qt::AlignCenter, initials(title));
    }

    if (hovered) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(rgba(255, 255, 255, 0.05));
        painter->drawRoundedRect(art, 3, 3);

        // Secondary to double click and the inspector: a quiet hint, not a control.
        const int badge = qBound(28, art.width() / 4, 52);
        const QRect play(art.center().x() - badge / 2, art.center().y() - badge / 2, badge, badge);
        painter->setBrush(rgba(10, 12, 16, 0.55));
        painter->drawEllipse(play);
        const int glyph = qRound(badge * 0.46);
        painter->drawPixmap(play.center() - QPoint(glyph / 2 - 1, glyph / 2),
                            Icons::pixmap(Icons::Name::Play, rgba(255, 255, 255, 0.92), glyph));
    }

    // A hairline directly on the artwork, accent when selected: no frame around
    // the whole card, nothing that reads as a form control.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(selected ? Theme::accent() : rgba(255, 255, 255, 0.10), 1));
    painter->drawRoundedRect(QRectF(art).adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);

    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::accent());
        painter->drawRoundedRect(QRect(art.left(), art.bottom() + 5, art.width(), 2), 1, 1);
    }

    const int titleSize = qBound(11, m_cardWidth * 13 / 186, 15);
    const int systemSize = qBound(9, m_cardWidth * 10 / 186, 12);

    QFont titleFont = option.font;
    titleFont.setPixelSize(titleSize);
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(Theme::textPrimary());
    const QRect titleRect(metadata.left() + 1, metadata.top() + (selected ? 10 : 8), metadata.width() - 2, titleSize + 5);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width()));

    QFont systemFont = option.font;
    systemFont.setPixelSize(systemSize);
    painter->setFont(systemFont);
    painter->setPen(Theme::textSecondary());
    // Cover, title, platform. Save state, size and dates belong to the inspector.
    const QRect systemRect(metadata.left() + 1, titleRect.bottom() + 1, metadata.width() - 2, systemSize + 5);
    painter->drawText(systemRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(systemName(system), Qt::ElideRight, systemRect.width()));

    painter->restore();
}

} // namespace Pocket::App
