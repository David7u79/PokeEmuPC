#pragma once

#include <QStyledItemDelegate>

namespace Pocket::App {

// One library card: cover, title, system. Kept as a delegate so the grid stays a
// plain QListView over the model.
class GameCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit GameCardDelegate(QObject* parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace Pocket::App
