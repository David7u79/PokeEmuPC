#pragma once

#include "ControllerArtworkLayer.hpp"

#include "pocket/input/ControllerLayout.hpp"

#include <QSize>
#include <QPixmap>
#include <QSet>

#include <memory>
#include <optional>

class QPainter;

namespace Pocket::Input {
class ControllerMapping;
}

namespace Pocket::App {

QString controlDisplayName(const QString& id);

class ControllerHintOverlay {
public:
    void setSystem(const QString& system);
    void setMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    bool isValid() const;

    QRectF artworkRect(const QSize& widgetSize) const;
    QRectF controlRect(const QString& id, const QSize& widgetSize) const;
    QRectF labelRectFor(const QString& id, const QSize& widgetSize) const;
    void paintFrame(QPainter& painter, const QSize& widgetSize) const;
    void setPressed(const QString& controlId, bool pressed);
    void clearPressed();
    bool isPressed(const QString& controlId) const;
    void setCaptureHighlight(const QString& controlId, bool active);
    void paintPressed(QPainter& painter, const QSize& widgetSize) const;
    void paintKeyLabels(QPainter& painter, const QSize& widgetSize) const;
    // Compatibility helper for callers that still use the former corner overlay.
    void paint(QPainter& painter, const QRect& bounds) const;
    QSize preferredSize(const QSize& available) const;

private:
    ControllerArtworkLayer m_artwork;
    std::optional<Pocket::Input::ControllerLayout> m_layout;
    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
    mutable QPixmap m_framePixmap;
    mutable QSize m_cachedSize;
    mutable qreal m_cachedDevicePixelRatio{0.0};
    mutable QString m_cachedSystem;
    mutable int m_rasterizationCount{0};
    QSet<QString> m_pressedControls;
    QSet<QString> m_captureControls;

public:
    int rasterizationCount() const { return m_rasterizationCount; }
};

enum class EmulatorViewMode { ConsoleFrame, FullScreen };

} // namespace Pocket::App
