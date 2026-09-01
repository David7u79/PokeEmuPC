#pragma once

#include <QObject>
#include <QTimer>

namespace PocketPartner::DesktopCompanion {

enum class RenderState {
    Hidden,              // 0 FPS (timers stopped)
    FullyStatic,         // 0 FPS (paint on demand / events only)
    MinorIdleAnimation,  // 5-10 FPS
    InteractiveAnimation // 20-30 FPS
};

class FramerateGovernor : public QObject {
    Q_OBJECT
public:
    explicit FramerateGovernor(QObject *parent = nullptr);

    void setRenderState(RenderState state);
    RenderState currentState() const { return m_state; }

    int targetFps() const;
    int intervalMs() const;

signals:
    void renderTick();

private slots:
    void onTimerTimeout();

private:
    RenderState m_state{RenderState::FullyStatic};
    QTimer m_timer;
};

} // namespace PocketPartner::DesktopCompanion
