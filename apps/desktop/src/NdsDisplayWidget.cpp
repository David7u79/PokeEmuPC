#include "NdsDisplayWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

namespace Pocket::App {

NdsDisplayWidget::NdsDisplayWidget(QWidget *parent)
    : QWidget(parent)
    , m_topImage(256, 192, QImage::Format_RGBA8888)
    , m_bottomImage(256, 192, QImage::Format_RGBA8888) {
    m_topImage.fill(Qt::darkBlue);
    m_bottomImage.fill(Qt::darkCyan);
}

void NdsDisplayWidget::setLayoutMode(NdsScreenLayout mode) {
    if (m_layoutMode != mode) {
        m_layoutMode = mode;
        update();
    }
}

void NdsDisplayWidget::updateFramebuffers(const uint8_t* topRgba, const uint8_t* bottomRgba) {
    if (topRgba) {
        std::memcpy(m_topImage.bits(), topRgba, 256 * 192 * 4);
    }
    if (bottomRgba) {
        std::memcpy(m_bottomImage.bits(), bottomRgba, 256 * 192 * 4);
    }
    update();
}

void NdsDisplayWidget::calculateScreenRects(const QRect& bounds, QRect& outTop, QRect& outBottom) const {
    int w = bounds.width();
    int h = bounds.height();

    switch (m_layoutMode) {
        case NdsScreenLayout::Vertical: {
            int subHeight = h / 2;
            outTop = QRect(bounds.x(), bounds.y(), w, subHeight);
            outBottom = QRect(bounds.x(), bounds.y() + subHeight, w, subHeight);
            break;
        }
        case NdsScreenLayout::Horizontal: {
            int subWidth = w / 2;
            outTop = QRect(bounds.x(), bounds.y(), subWidth, h);
            outBottom = QRect(bounds.x() + subWidth, bounds.y(), subWidth, h);
            break;
        }
        case NdsScreenLayout::FocusedTop: {
            outTop = bounds;
            outBottom = QRect();
            break;
        }
        case NdsScreenLayout::FocusedBottom: {
            outTop = QRect();
            outBottom = bounds;
            break;
        }
    }
}

void NdsDisplayWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    calculateScreenRects(rect(), m_currentTopRect, m_currentBottomRect);

    if (!m_currentTopRect.isEmpty()) {
        painter.drawImage(m_currentTopRect, m_topImage);
    }
    if (!m_currentBottomRect.isEmpty()) {
        painter.drawImage(m_currentBottomRect, m_bottomImage);
    }
}

void NdsDisplayWidget::mousePressEvent(QMouseEvent *event) {
    processTouchEvent(event->pos(), true);
}

void NdsDisplayWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        processTouchEvent(event->pos(), true);
    }
}

void NdsDisplayWidget::mouseReleaseEvent(QMouseEvent *event) {
    processTouchEvent(event->pos(), false);
}

void NdsDisplayWidget::processTouchEvent(const QPoint& mousePos, bool isPressed) {
    if (m_currentBottomRect.isEmpty() || !m_currentBottomRect.contains(mousePos)) {
        if (!isPressed) {
            emit touchInputChanged(0, 0, false);
        }
        return;
    }

    int relX = mousePos.x() - m_currentBottomRect.x();
    int relY = mousePos.y() - m_currentBottomRect.y();

    int touchX = std::clamp((relX * 256) / m_currentBottomRect.width(), 0, 255);
    int touchY = std::clamp((relY * 192) / m_currentBottomRect.height(), 0, 191);

    emit touchInputChanged(touchX, touchY, isPressed);
}

} // namespace Pocket::App
