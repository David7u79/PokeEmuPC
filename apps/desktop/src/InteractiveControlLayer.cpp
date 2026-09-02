#include "InteractiveControlLayer.hpp"

#include <QFontMetrics>
#include <QPainter>

namespace Pocket::App {
namespace {
int priority(ControlVisualState state)
{
    switch (state) {
    case ControlVisualState::CAPTURING: return 6;
    case ControlVisualState::CONFLICT: return 5;
    case ControlVisualState::SELECTED: return 4;
    case ControlVisualState::HOVER: return 3;
    case ControlVisualState::MAPPED: return 2;
    case ControlVisualState::NORMAL: return 1;
    }
    return 0;
}
QColor colorFor(ControlVisualState state)
{
    switch (state) {
    case ControlVisualState::CAPTURING: return QColor(255, 193, 7);
    case ControlVisualState::CONFLICT: return QColor(220, 53, 69);
    case ControlVisualState::SELECTED: return QColor(33, 150, 243);
    case ControlVisualState::HOVER: return QColor(0, 188, 212);
    case ControlVisualState::MAPPED: return QColor(76, 175, 80);
    default: return QColor(158, 158, 158);
    }
}
}

QRectF InteractiveControlLayer::rectFor(const Pocket::Input::ControllerControl& control, const QRectF& target) const
{
    return {target.left() + control.x * target.width(), target.top() + control.y * target.height(),
            control.width * target.width(), control.height * target.height()};
}

const Pocket::Input::ControllerControl* InteractiveControlLayer::hitTest(const QPointF& widgetPos, const QRectF& target) const
{
    if (!m_layout || target.isEmpty() || !target.contains(widgetPos)) return nullptr;
    return m_layout->controlAt((widgetPos.x() - target.left()) / target.width(),
                               (widgetPos.y() - target.top()) / target.height());
}

void InteractiveControlLayer::paint(QPainter& painter, const QRectF& target, const QRect& bounds,
                                    const QHash<QString, ControlVisualState>& states,
                                    const QHash<QString, QString>& labels) const
{
    if (!m_layout) return;
    painter.save();
    const QFontMetrics metrics(painter.font());
    for (const auto& control : m_layout->controls()) {
        ControlVisualState state = states.value(control.id, ControlVisualState::NORMAL);
        const QColor color = colorFor(state);
        const QRectF controlRect = rectFor(control, target);
        painter.setPen(QPen(color, 2.0));
        QColor fill = color; fill.setAlpha(70);
        painter.setBrush(fill);
        painter.drawRoundedRect(controlRect, 4, 4);
        const QString label = labels.value(control.id);
        if (label.isEmpty()) continue;
        const int roomRight = bounds.right() - qRound(controlRect.right());
        const int roomLeft = qRound(controlRect.left()) - bounds.left();
        const bool right = roomRight >= roomLeft;
        const int available = qMax(0, (right ? roomRight : roomLeft) - 6);
        const QString text = metrics.elidedText(label, Qt::ElideRight, available);
        if (text.isEmpty()) continue;
        const int x = right ? qRound(controlRect.right()) + 4 : qRound(controlRect.left()) - 4 - metrics.horizontalAdvance(text);
        const int y = qBound(bounds.top() + metrics.ascent(), qRound(controlRect.center().y()) + metrics.ascent() / 2, bounds.bottom());
        painter.setPen(Qt::white);
        painter.drawText(x, y, text);
    }
    painter.restore();
}

} // namespace Pocket::App
