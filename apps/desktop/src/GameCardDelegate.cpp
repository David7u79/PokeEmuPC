#include "GameCardDelegate.hpp"

#include <QPainter>
#include <QTextLayout>

namespace {

constexpr int ArtworkRole = Qt::UserRole + 3;

QString initials(const QString& title)
{
    QString result;
    for (const QString& word : title.split(' ', Qt::SkipEmptyParts)) {
        result += word.left(1).toUpper();
        if (result.size() == 2) break;
    }
    return result.isEmpty() ? QStringLiteral("?") : result;
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
    return {m_cardWidth, qRound(m_cardWidth * 232.0 / 176.0)};
}

void GameCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    const QRect card = option.rect.adjusted(1, 1, -1, -1);
    const QPalette& palette = option.palette;
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    QColor border = selected ? palette.highlight().color() : palette.mid().color();
    QColor background = palette.base().color();
    if (hovered && !selected) {
        background = palette.alternateBase().color();
        border = palette.highlight().color();
        border.setAlpha(128);
    }
    painter->setBrush(background);
    painter->setPen(QPen(border, selected ? 2 : 1));
    painter->drawRoundedRect(card, 8, 8);

    const int coverSide = m_cardWidth - 24;
    const QRect cover(card.left() + 12, card.top() + 10, coverSide, coverSide);
    const QString title = index.data().toString();
    const QString artworkPath = index.data(ArtworkRole).toString();
    QPixmap pixmap(artworkPath);
    if (!artworkPath.isEmpty() && !pixmap.isNull()) {
        const QSize scaledSize = pixmap.size().scaled(cover.size(), Qt::KeepAspectRatio);
        painter->drawPixmap(QRect(cover.center() - QPoint(scaledSize.width() / 2, scaledSize.height() / 2), scaledSize), pixmap);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(palette.midlight());
        painter->drawRoundedRect(cover, 6, 6);
        QFont font = option.font;
        font.setPointSize(qMax(1, qRound(font.pointSize() * m_cardWidth / 176.0) + 10));
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(palette.text().color());
        painter->drawText(cover, Qt::AlignCenter, initials(title));
    }

    QFont titleFont = option.font;
    titleFont.setPointSize(qMax(1, qRound(titleFont.pointSize() * m_cardWidth / 176.0)));
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(palette.text().color());
    const QRect titleRect(card.left() + 10, cover.bottom() + 7, card.width() - 20, qRound(34.0 * m_cardWidth / 176.0));
    QTextOption textOption(Qt::AlignHCenter);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextLayout layout(title, titleFont);
    layout.setTextOption(textOption);
    layout.beginLayout();
    int y = titleRect.top();
    for (int lineNumber = 0; lineNumber < 2; ++lineNumber) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) break;
        line.setLineWidth(titleRect.width());
        line.setPosition({qreal(titleRect.left()), qreal(y)});
        y += qRound(line.height());
    }
    layout.endLayout();
    layout.draw(painter, QPoint());

    QFont systemFont = option.font;
    systemFont.setPointSize(qMax(1, qRound((systemFont.pointSize() - 1.0) * m_cardWidth / 176.0)));
    painter->setFont(systemFont);
    painter->setPen(palette.mid().color());
    painter->drawText(QRect(card.left() + 10, card.bottom() - qRound(25.0 * m_cardWidth / 176.0), card.width() - 20, qRound(18.0 * m_cardWidth / 176.0)), Qt::AlignHCenter | Qt::AlignVCenter, index.data(Qt::UserRole + 2).toString());
    painter->restore();
}

} // namespace Pocket::App
