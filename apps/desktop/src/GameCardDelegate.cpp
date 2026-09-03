#include "GameCardDelegate.hpp"

#include "Theme.hpp"

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
    return {m_cardWidth, m_cardWidth + 42};
}

void GameCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    QRect card = option.rect.adjusted(1, 1, -1, -1);
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    if (hovered) card.translate(0, -2);
    const QRect shadow = card.translated(0, 2);
    QColor shadowColor(Qt::black);
    shadowColor.setAlphaF(0.12);
    painter->setPen(Qt::NoPen);
    painter->setBrush(shadowColor);
    painter->drawRoundedRect(shadow, 10, 10);

    const QRect cover(card.left(), card.top(), card.width(), card.width());
    const QRect textArea(card.left(), cover.bottom() + 1, card.width(), card.bottom() - cover.bottom());
    const QString title = index.data().toString();
    const QString system = index.data(SystemRole).toString();
    const QString artworkPath = index.data(ArtworkRole).toString();
    QPixmap pixmap(artworkPath);
    QColor cardBackground = Theme::surfaceRaised();
    if (selected) {
        cardBackground = cardBackground.lighter(108);
    }
    painter->setPen(QPen(Theme::border(), 1));
    painter->setBrush(cardBackground);
    painter->drawRoundedRect(card, 8, 8);

    painter->save();
    QPainterPath coverClip;
    coverClip.addRoundedRect(cover, 8, 8);
    painter->setClipPath(coverClip);
    if (!artworkPath.isEmpty() && !pixmap.isNull()) {
        // Whole cover, never cropped: the box is part of what the player recognises.
        // Any space it leaves is filled by the console's own colour.
        QLinearGradient backdrop(cover.topLeft(), cover.bottomRight());
        const QColor color = systemColor(system);
        backdrop.setColorAt(0, color.darker(115));
        backdrop.setColorAt(1, color.darker(150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(backdrop);
        painter->drawRect(cover);
        const QSize scaledSize = pixmap.size().scaled(cover.size() - QSize(8, 8), Qt::KeepAspectRatio);
        painter->drawPixmap(QRect(cover.center() - QPoint(scaledSize.width() / 2, scaledSize.height() / 2), scaledSize), pixmap);
    } else {
        QLinearGradient gradient(cover.topLeft(), cover.bottomRight());
        const QColor color = systemColor(system);
        gradient.setColorAt(0, color.lighter(120));
        gradient.setColorAt(1, color.darker(115));
        painter->setPen(Qt::NoPen);
        painter->setBrush(gradient);
        painter->drawRect(cover);
        QFont font = option.font;
        font.setPixelSize(qMax(16, cover.width() / 3));
        font.setBold(true);
        painter->setFont(font);
        QColor initialsColor(Qt::white);
        initialsColor.setAlphaF(0.70);
        painter->setPen(initialsColor);
        painter->drawText(cover, Qt::AlignCenter, initials(title));
    }

    painter->restore();

    QFont titleFont = option.font;
    titleFont.setPixelSize(qMax(9, 12 * m_cardWidth / 176));
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(Theme::textPrimary());
    const QRect titleRect(textArea.left() + 8, textArea.top() + 4, textArea.width() - 16, 17);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width()));

    QFont systemFont = option.font;
    systemFont.setPixelSize(qMax(8, 9 * m_cardWidth / 176));
    painter->setFont(systemFont);
    painter->setPen(Theme::textSecondary());
    painter->drawText(QRect(textArea.left() + 8, textArea.top() + 21, textArea.width() - 16, 15), Qt::AlignLeft | Qt::AlignVCenter, system);

    QColor border = Theme::accent();
    if (selected) {
        painter->setPen(QPen(border, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(card, 8, 8);
    } else if (hovered) {
        border.setAlphaF(0.40);
        painter->setPen(QPen(border, 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(card, 8, 8);
    }
    painter->restore();
}

} // namespace Pocket::App
