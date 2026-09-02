#include "NdsDisplayTransform.hpp"

#include <algorithm>
#include <cmath>

namespace Pocket::App {

void NdsDisplayTransform::setLayout(NdsScreenLayout layout) {
    m_layout = layout;
    updateRects();
}

void NdsDisplayTransform::setViewport(const QSize& widgetSize, qreal devicePixelRatio) {
    m_viewport = widgetSize.expandedTo(QSize());
    m_devicePixelRatio = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    updateRects();
}

void NdsDisplayTransform::setScreenSize(const QSize& screenSize) {
    if (screenSize.width() <= 0 || screenSize.height() <= 0)
        return;
    m_screenSize = screenSize;
    updateRects();
}

QRect NdsDisplayTransform::topRect() const {
    return m_topRect;
}
QRect NdsDisplayTransform::bottomRect() const {
    return m_bottomRect;
}

NdsScreen NdsDisplayTransform::screenAt(const QPoint& widgetPos) const {
    if (!QRect(QPoint(), m_viewport).contains(widgetPos))
        return NdsScreen::None;
    if (m_topRect.contains(widgetPos))
        return NdsScreen::Top;
    if (m_bottomRect.contains(widgetPos))
        return NdsScreen::Bottom;
    return NdsScreen::None;
}

std::optional<QPoint> NdsDisplayTransform::touchAt(const QPoint& widgetPos) const {
    if (screenAt(widgetPos) != NdsScreen::Bottom || m_bottomRect.isEmpty())
        return std::nullopt;
    const int x = std::clamp((widgetPos.x() - m_bottomRect.left()) * m_screenSize.width() / m_bottomRect.width(), 0,
                             m_screenSize.width() - 1);
    const int y = std::clamp((widgetPos.y() - m_bottomRect.top()) * m_screenSize.height() / m_bottomRect.height(), 0,
                             m_screenSize.height() - 1);
    return QPoint(x, y);
}

QRect NdsDisplayTransform::fittedRect(const QRect& available) const {
    if (available.isEmpty())
        return {};
    const qreal scale = std::min(static_cast<qreal>(available.width()) / m_screenSize.width(),
                                 static_cast<qreal>(available.height()) / m_screenSize.height());
    const int width = std::max(1, qRound(m_screenSize.width() * scale));
    const int height = std::max(1, qRound(m_screenSize.height() * scale));
    return QRect(available.x() + (available.width() - width) / 2, available.y() + (available.height() - height) / 2,
                 width, height);
}

void NdsDisplayTransform::updateRects() {
    m_topRect = {};
    m_bottomRect = {};
    const QRect bounds(QPoint(), m_viewport);
    if (bounds.isEmpty())
        return;

    switch (m_layout) {
    case NdsScreenLayout::Vertical: {
        const qreal scale = std::min(static_cast<qreal>(bounds.width()) / m_screenSize.width(),
                                     static_cast<qreal>(bounds.height()) / (2 * m_screenSize.height()));
        const QSize size(std::max(1, qRound(m_screenSize.width() * scale)),
                         std::max(1, qRound(m_screenSize.height() * scale)));
        const int x = (bounds.width() - size.width()) / 2;
        const int y = (bounds.height() - 2 * size.height()) / 2;
        m_topRect = QRect(x, y, size.width(), size.height());
        m_bottomRect = QRect(x, y + size.height(), size.width(), size.height());
        break;
    }
    case NdsScreenLayout::Horizontal: {
        const qreal scale = std::min(static_cast<qreal>(bounds.width()) / (2 * m_screenSize.width()),
                                     static_cast<qreal>(bounds.height()) / m_screenSize.height());
        const QSize size(std::max(1, qRound(m_screenSize.width() * scale)),
                         std::max(1, qRound(m_screenSize.height() * scale)));
        const int x = (bounds.width() - 2 * size.width()) / 2;
        const int y = (bounds.height() - size.height()) / 2;
        m_topRect = QRect(x, y, size.width(), size.height());
        m_bottomRect = QRect(x + size.width(), y, size.width(), size.height());
        break;
    }
    case NdsScreenLayout::FocusedTop:
        m_topRect = fittedRect(QRect(0, 0, bounds.width(), bounds.height() * 3 / 4));
        m_bottomRect = fittedRect(QRect(0, bounds.height() * 3 / 4, bounds.width(), bounds.height() / 4));
        break;
    case NdsScreenLayout::FocusedBottom:
        m_topRect = fittedRect(QRect(0, 0, bounds.width(), bounds.height() / 4));
        m_bottomRect = fittedRect(QRect(0, bounds.height() / 4, bounds.width(), bounds.height() * 3 / 4));
        break;
    }
}

} // namespace Pocket::App
