#include "GameCardDelegate.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

namespace {

constexpr int ArtworkRole = Qt::UserRole + 3;
constexpr int SystemRole = Qt::UserRole + 2;

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

QSize GameCardDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return {m_cardWidth, m_cardWidth * 4 / 3 + 34};
}

void GameCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    QRect card = option.rect.adjusted(1, 1, -1, -1);
    const QPalette& palette = option.palette;
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    if (hovered) card.translate(0, -2);
    const QRect shadow = card.translated(0, 2);
    QColor shadowColor(Qt::black);
    shadowColor.setAlphaF(0.12);
    painter->setPen(Qt::NoPen);
    painter->setBrush(shadowColor);
    painter->drawRoundedRect(shadow, 10, 10);

    const QRect cover(card.left(), card.top(), card.width(), card.height() - 34);
    const QRect bottomBar(card.left(), cover.bottom() + 1, card.width(), 34);
    const QString title = index.data().toString();
    const QString system = index.data(SystemRole).toString();
    const QString artworkPath = index.data(ArtworkRole).toString();
    QPixmap pixmap(artworkPath);
    if (!artworkPath.isEmpty() && !pixmap.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(cover, 10, 10);
        painter->save();
        painter->setClipPath(clip);
        const QSize scaledSize = pixmap.size().scaled(cover.size(), Qt::KeepAspectRatioByExpanding);
        painter->drawPixmap(QRect(cover.center() - QPoint(scaledSize.width() / 2, scaledSize.height() / 2), scaledSize), pixmap);
        painter->restore();
    } else {
        QLinearGradient gradient(cover.topLeft(), cover.bottomRight());
        const QColor color = systemColor(system);
        gradient.setColorAt(0, color.lighter(120));
        gradient.setColorAt(1, color.darker(115));
        painter->setPen(Qt::NoPen);
        painter->setBrush(gradient);
        painter->drawRoundedRect(cover, 10, 10);
        QFont font = option.font;
        font.setPixelSize(qMax(16, cover.width() / 3));
        font.setBold(true);
        painter->setFont(font);
        QColor initialsColor(Qt::white);
        initialsColor.setAlphaF(0.70);
        painter->setPen(initialsColor);
        painter->drawText(cover, Qt::AlignCenter, initials(title));
    }

    QFont badgeFont = option.font;
    badgeFont.setPixelSize(9);
    painter->setFont(badgeFont);
    const QRect badgeText = painter->fontMetrics().boundingRect(system);
    const QRect badge(cover.right() - badgeText.width() - 16, cover.top() + 6, badgeText.width() + 10, badgeText.height() + 6);
    QColor badgeBackground(Qt::black);
    badgeBackground.setAlphaF(0.55);
    painter->setPen(Qt::NoPen);
    painter->setBrush(badgeBackground);
    painter->drawRoundedRect(badge, badge.height() / 2, badge.height() / 2);
    painter->setPen(Qt::white);
    painter->drawText(badge, Qt::AlignCenter, system);

    painter->setPen(Qt::NoPen);
    painter->setBrush(palette.base());
    painter->drawRect(bottomBar);

    QFont titleFont = option.font;
    titleFont.setPixelSize(qMax(9, 12 * m_cardWidth / 176));
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(palette.text().color());
    const QRect titleRect(bottomBar.left() + 8, bottomBar.top() + 2, bottomBar.width() - 16, 16);
    painter->drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width()));

    QFont systemFont = option.font;
    systemFont.setPixelSize(qMax(8, 9 * m_cardWidth / 176));
    painter->setFont(systemFont);
    painter->setPen(palette.mid().color());
    painter->drawText(QRect(bottomBar.left() + 8, bottomBar.top() + 17, bottomBar.width() - 16, 15), Qt::AlignHCenter | Qt::AlignVCenter, system);

    QColor border = palette.highlight().color();
    if (selected) {
        painter->setPen(QPen(border, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(card, 10, 10);
        painter->setPen(Qt::NoPen);
        painter->setBrush(border);
        painter->drawRect(card.left(), card.bottom() - 2, card.width(), 3);
    } else if (hovered) {
        border.setAlphaF(0.40);
        painter->setPen(QPen(border, 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(card, 10, 10);
    }
    painter->restore();
}

} // namespace Pocket::App
