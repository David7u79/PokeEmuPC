#include "ControllerArtworkLayer.hpp"

#include "pocket/input/ControllerLayout.hpp"
#include <QPainter>

namespace Pocket::App {

bool ControllerArtworkLayer::setSystem(const QString& system)
{
    if (m_system == system) return m_renderer.isValid();
    m_system = system;
    const auto layout = Pocket::Input::ControllerLayout::forSystem(system);
    m_renderer.load(layout ? layout->artworkFile() : QString());
    return m_renderer.isValid();
}

QRectF ControllerArtworkLayer::targetRect(const QSize& widgetSize) const
{
    if (widgetSize.isEmpty()) return {};
    const QRectF viewBox = m_renderer.viewBoxF();
    if (viewBox.isEmpty()) return QRectF(QPointF(), QSizeF(widgetSize));
    const qreal scale = qMin(widgetSize.width() / viewBox.width(), widgetSize.height() / viewBox.height());
    const QSizeF size(viewBox.width() * scale, viewBox.height() * scale);
    return QRectF((widgetSize.width() - size.width()) / 2.0,
                  (widgetSize.height() - size.height()) / 2.0, size.width(), size.height());
}

void ControllerArtworkLayer::render(QPainter& painter, const QRectF& target) const
{
    if (m_renderer.isValid()) m_renderer.render(&painter, target);
}

void ControllerArtworkLayer::render(QPainter& painter, const QRectF& target)
{
    static_cast<const ControllerArtworkLayer*>(this)->render(painter, target);
}

} // namespace Pocket::App
