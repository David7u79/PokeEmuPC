#pragma once

#include "pocket/input/ControllerLayout.hpp"
#include <QHash>
#include <QRectF>
#include <optional>

class QPainter;

namespace Pocket::App {

enum class ControlVisualState { NORMAL, HOVER, SELECTED, CAPTURING, MAPPED, CONFLICT };

class InteractiveControlLayer {
public:
    void setLayout(const Pocket::Input::ControllerLayout& layout) { m_layout = layout; }
    bool hasLayout() const { return m_layout.has_value(); }
    QRectF rectFor(const Pocket::Input::ControllerControl& control, const QRectF& target) const;
    const Pocket::Input::ControllerControl* hitTest(const QPointF& widgetPos, const QRectF& target) const;
    void paint(QPainter& painter, const QRectF& target, const QRect& bounds,
               const QHash<QString, ControlVisualState>& states,
               const QHash<QString, QString>& labels) const;

private:
    std::optional<Pocket::Input::ControllerLayout> m_layout;
};

} // namespace Pocket::App
