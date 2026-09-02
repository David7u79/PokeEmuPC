#pragma once

#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <optional>

namespace Pocket::App {

enum class NdsScreenLayout { Vertical, Horizontal, FocusedTop, FocusedBottom };
enum class NdsScreen { None, Top, Bottom };

class NdsDisplayTransform {
public:
    void setLayout(NdsScreenLayout layout);
    void setViewport(const QSize& widgetSize, qreal devicePixelRatio = 1.0);
    void setScreenSize(const QSize& screenSize);
    void setTouchScreenRect(const QRectF& rect);
    void clearTouchScreenRect();

    QRect topRect() const;
    QRect bottomRect() const;
    NdsScreen screenAt(const QPoint& widgetPos) const;
    std::optional<QPoint> touchAt(const QPoint& widgetPos) const;

private:
    QRect fittedRect(const QRect& available) const;
    void updateRects();

    NdsScreenLayout m_layout{NdsScreenLayout::Vertical};
    QSize m_viewport;
    QSize m_screenSize{256, 192};
    qreal m_devicePixelRatio{1.0};
    QRect m_topRect;
    QRect m_bottomRect;
    QRectF m_touchScreenRect;
};

} // namespace Pocket::App
