#pragma once

#include <QImage>
#include <QHash>
#include <QKeyEvent>
#include <QRect>
#include <QWidget>
#include <QTimer>
#include "ControllerHintOverlay.hpp"
#include "NdsDisplayTransform.hpp"
#include "pocket/emulator/EmulatorEngine.hpp"
#include "pocket/input/ControllerMapping.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace Pocket::App {

class NdsDisplayWidget : public QWidget {
    Q_OBJECT
public:
    explicit NdsDisplayWidget(QWidget* parent = nullptr);
    ~NdsDisplayWidget() override = default;

    void setLayoutMode(NdsScreenLayout mode);
    NdsScreenLayout layoutMode() const { return m_layoutMode; }

    void updateFramebuffers(const uint8_t* topRgba, const uint8_t* bottomRgba);
    void submitCombinedFrame(const uint8_t* pixels, int width, int height, size_t pitch);
    void setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    void refreshKeyBindings();
    bool hintsVisible() const { return m_hintsVisible; }
    void setHintsVisible(bool visible);
    void toggleHints();
    EmulatorViewMode viewMode() const { return m_viewMode; }
    void setViewMode(EmulatorViewMode mode);
    void toggleViewMode();

    // Calculate bounding rects for current widget size
    void calculateScreenRects(const QRect& totalBounds, QRect& outTopRect, QRect& outBottomRect) const;

signals:
    void touchInputChanged(int touchX, int touchY, bool isPressed);
    void buttonInputChanged(Pocket::Emulator::EmulatorButton button, bool pressed);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void processTouchEvent(const QPoint& mousePos, bool isPressed);
    void releaseMouseControl();

    NdsScreenLayout m_layoutMode{NdsScreenLayout::Vertical};
    NdsDisplayTransform m_transform;

    QImage m_topImage;
    QImage m_bottomImage;
    std::mutex m_frameMutex;
    std::atomic_bool m_framesEnabled{false};
    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
    QHash<int, Pocket::Emulator::EmulatorButton> m_keyBindings;
    QHash<int, QString> m_keyControlIds;
    ControllerHintOverlay m_hintOverlay;
    bool m_hintsVisible{true};
    EmulatorViewMode m_viewMode{EmulatorViewMode::ConsoleFrame};

    QRect m_currentTopRect;
    QRect m_currentBottomRect;
    QString m_mousePressedControlId;
    bool m_touchInputActive{false};
};

} // namespace Pocket::App
