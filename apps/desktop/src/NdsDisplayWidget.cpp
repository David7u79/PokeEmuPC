#include "NdsDisplayWidget.hpp"

#include "pocket/input/ControllerLayout.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <cstring>

namespace Pocket::App {

NdsDisplayWidget::NdsDisplayWidget(QWidget* parent)
    : QWidget(parent), m_topImage(256, 192, QImage::Format_RGB32), m_bottomImage(256, 192, QImage::Format_RGB32) {
    m_topImage.fill(Qt::darkBlue);
    m_bottomImage.fill(Qt::darkCyan);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(256, 384);
    m_hintOverlay.setSystem(QStringLiteral("NDS"));
    QSettings settings("PocketPartnerProject", "PocketPartner");
    m_hintsVisible = settings.value("emulator/showControlHints", true).toBool();
    m_viewMode = settings.value("emulator/viewMode", 0).toInt() == 1 ? EmulatorViewMode::FullScreen
                                                                      : EmulatorViewMode::ConsoleFrame;
}

void NdsDisplayWidget::setLayoutMode(NdsScreenLayout mode) {
    if (m_layoutMode == mode)
        return;
    m_layoutMode = mode;
    m_transform.setLayout(mode);
    update();
}

void NdsDisplayWidget::updateFramebuffers(const uint8_t* topRgba, const uint8_t* bottomRgba) {
    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (topRgba)
        std::memcpy(m_topImage.bits(), topRgba, m_topImage.sizeInBytes());
    if (bottomRgba)
        std::memcpy(m_bottomImage.bits(), bottomRgba, m_bottomImage.sizeInBytes());
    update();
}

void NdsDisplayWidget::submitCombinedFrame(const uint8_t* pixels, int width, int height, size_t pitch) {
    if (!m_framesEnabled.load(std::memory_order_relaxed) || !pixels || width <= 0 || height < 2 || pitch == 0)
        return;
    std::lock_guard<std::mutex> lock(m_frameMutex);
    const int screenHeight = height / 2;
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    if (pitch < rowBytes)
        return;
    if (m_topImage.size() != QSize(width, screenHeight)) {
        m_topImage = QImage(width, screenHeight, QImage::Format_RGB32);
        m_bottomImage = QImage(width, screenHeight, QImage::Format_RGB32);
        m_transform.setScreenSize(QSize(width, screenHeight));
    }
    for (int y = 0; y < screenHeight; ++y) {
        std::memcpy(m_topImage.scanLine(y), pixels + static_cast<size_t>(y) * pitch, rowBytes);
        std::memcpy(m_bottomImage.scanLine(y), pixels + static_cast<size_t>(y + screenHeight) * pitch, rowBytes);
    }
    QMetaObject::invokeMethod(
        this, [this] { update(); }, Qt::QueuedConnection);
}

void NdsDisplayWidget::calculateScreenRects(const QRect& bounds, QRect& outTop, QRect& outBottom) const {
    // Compatibility facade retained for callers that still request the former
    // whole-area layout. Painting and hit-testing use NdsDisplayTransform.
    switch (m_layoutMode) {
    case NdsScreenLayout::Vertical:
        outTop = QRect(bounds.x(), bounds.y(), bounds.width(), bounds.height() / 2);
        outBottom = QRect(bounds.x(), bounds.y() + bounds.height() / 2, bounds.width(), bounds.height() / 2);
        break;
    case NdsScreenLayout::Horizontal:
        outTop = QRect(bounds.x(), bounds.y(), bounds.width() / 2, bounds.height());
        outBottom = QRect(bounds.x() + bounds.width() / 2, bounds.y(), bounds.width() / 2, bounds.height());
        break;
    case NdsScreenLayout::FocusedTop:
        outTop = bounds;
        outBottom = {};
        break;
    case NdsScreenLayout::FocusedBottom:
        outTop = {};
        outBottom = bounds;
        break;
    }
}

void NdsDisplayWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    const bool framed = m_viewMode == EmulatorViewMode::ConsoleFrame && m_hintOverlay.isValid();
    if (framed) {
        m_hintOverlay.paintFrame(painter, size());
        m_currentTopRect = m_hintOverlay.controlRect(QStringLiteral("SCREEN_TOP"), size()).toAlignedRect();
        m_currentBottomRect = m_hintOverlay.controlRect(QStringLiteral("TOUCHSCREEN"), size()).toAlignedRect();
        m_transform.setTouchScreenRect(QRectF(m_currentBottomRect));
    } else {
        m_transform.clearTouchScreenRect();
        m_transform.setViewport(size(), devicePixelRatioF());
        m_currentTopRect = m_transform.topRect();
        m_currentBottomRect = m_transform.bottomRect();
    }
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        painter.drawImage(m_currentTopRect, m_topImage);
        painter.drawImage(m_currentBottomRect, m_bottomImage);
    }
    if (framed && m_hintsVisible)
        m_hintOverlay.paintKeyLabels(painter, size());
}

void NdsDisplayWidget::setViewMode(EmulatorViewMode mode) {
    if (m_viewMode == mode)
        return;
    m_viewMode = mode;
    QSettings("PocketPartnerProject", "PocketPartner").setValue("emulator/viewMode", mode == EmulatorViewMode::FullScreen ? 1 : 0);
    update();
}

void NdsDisplayWidget::toggleViewMode() {
    setViewMode(m_viewMode == EmulatorViewMode::ConsoleFrame ? EmulatorViewMode::FullScreen
                                                              : EmulatorViewMode::ConsoleFrame);
}

void NdsDisplayWidget::mousePressEvent(QMouseEvent* event) {
    processTouchEvent(event->pos(), true);
}

void NdsDisplayWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton)
        processTouchEvent(event->pos(), true);
}

void NdsDisplayWidget::mouseReleaseEvent(QMouseEvent* event) {
    processTouchEvent(event->pos(), false);
}

void NdsDisplayWidget::processTouchEvent(const QPoint& mousePos, bool isPressed) {
    const auto touch = m_transform.touchAt(mousePos);
    if (touch) {
        emit touchInputChanged(touch->x(), touch->y(), isPressed);
    } else if (!isPressed) {
        emit touchInputChanged(0, 0, false);
    }
}

void NdsDisplayWidget::setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping) {
    m_mapping = std::move(mapping);
    m_hintOverlay.setMapping(m_mapping);
    refreshKeyBindings();
}

void NdsDisplayWidget::setHintsVisible(bool visible) {
    if (m_hintsVisible == visible)
        return;
    m_hintsVisible = visible;
    // Remembered, so the choice survives closing the app.
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/showControlHints", visible);
    update();
}

void NdsDisplayWidget::toggleHints() {
    setHintsVisible(!m_hintsVisible);
}

void NdsDisplayWidget::refreshKeyBindings() {
    m_keyBindings.clear();
    if (!m_mapping)
        return;
    const auto layout = Pocket::Input::ControllerLayout::forSystem(QStringLiteral("NDS"));
    if (!layout)
        return;
    for (const auto& control : layout->controls()) {
        const auto button = Pocket::Input::ControllerMapping::emulatorButtonFor(control.id);
        const auto binding = m_mapping->binding(QStringLiteral("NDS"), control.id);
        if (button && binding && binding->device == Pocket::Input::InputDevice::Keyboard) {
            m_keyBindings.insert(binding->code, *button);
        }
    }
}

void NdsDisplayWidget::keyPressEvent(QKeyEvent* event) {
    // F1 is reserved for the overlay even if a future mapping assigns it to a game button.
    if (event->key() == Qt::Key_F1) {
        if (!event->isAutoRepeat())
            toggleHints();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        if (!event->isAutoRepeat())
            toggleViewMode();
        event->accept();
        return;
    }
    if (!event->isAutoRepeat() && m_keyBindings.contains(event->key())) {
        emit buttonInputChanged(m_keyBindings.value(event->key()), true);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void NdsDisplayWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F1) {
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        event->accept();
        return;
    }
    if (!event->isAutoRepeat() && m_keyBindings.contains(event->key())) {
        emit buttonInputChanged(m_keyBindings.value(event->key()), false);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void NdsDisplayWidget::showEvent(QShowEvent* event) {
    m_framesEnabled.store(true, std::memory_order_relaxed);
    QWidget::showEvent(event);
}

void NdsDisplayWidget::hideEvent(QHideEvent* event) {
    m_framesEnabled.store(false, std::memory_order_relaxed);
    QWidget::hideEvent(event);
}

} // namespace Pocket::App
