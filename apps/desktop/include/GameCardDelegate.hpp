#pragma once

#include <QSizeF>
#include <QStyledItemDelegate>

namespace Pocket::App {

class GameCardDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit GameCardDelegate(QObject* parent = nullptr);

    void setCardWidth(int width);
    // Tallest cover in the library, as height/width. The cell is built from real
    // artwork instead of a guessed portrait ratio, so no row carries dead space.
    void setCoverAspect(double aspect);
    double coverAspect() const { return m_coverAspect; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int stageWidth() const;
    int stageHeight() const;
    int metadataHeight() const;
    QSizeF artSize(double aspect) const;

    int m_cardWidth{186};
    double m_coverAspect{1.05};
};

} // namespace Pocket::App
