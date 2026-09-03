#pragma once
#include <QStyledItemDelegate>
namespace Pocket::App { class GameCardDelegate : public QStyledItemDelegate { Q_OBJECT public: explicit GameCardDelegate(QObject *parent = nullptr); QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override; void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override; }; }
