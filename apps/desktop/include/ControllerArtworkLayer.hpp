#pragma once

#include <QRectF>
#include <QString>
#include <QSvgRenderer>

class QPainter;

namespace Pocket::App {

class ControllerArtworkLayer {
public:
    bool setSystem(const QString& system);
    bool isValid() const { return m_renderer.isValid(); }
    QString system() const { return m_system; }
    QRectF targetRect(const QSize& widgetSize) const;
    void render(QPainter& painter, const QRectF& target) const;
    void render(QPainter& painter, const QRectF& target);

private:
    QString m_system;
    mutable QSvgRenderer m_renderer;
};

} // namespace Pocket::App
