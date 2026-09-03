#pragma once

#include <QStyledItemDelegate>

namespace Pocket::App {

class GameCardDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit GameCardDelegate(QObject* parent = nullptr);

    void setCardWidth(int width);
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int m_cardWidth{176};
};

} // namespace Pocket::App
