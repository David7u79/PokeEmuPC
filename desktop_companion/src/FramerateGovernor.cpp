#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

namespace PocketPartner::DesktopCompanion {

FramerateGovernor::FramerateGovernor(QObject *parent)
    : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &FramerateGovernor::onTimerTimeout);
}

int FramerateGovernor::targetFps() const {
    switch (m_state) {
        case RenderState::Hidden:
        case RenderState::FullyStatic:
            return 0;
        case RenderState::MinorIdleAnimation:
            return 8; // 8 FPS for minor idle animation
        case RenderState::InteractiveAnimation:
            return 25; // 25 FPS for interactive animation
    }
    return 0;
}

int FramerateGovernor::intervalMs() const {
    int fps = targetFps();
    if (fps <= 0) return 0;
    return 1000 / fps;
}

void FramerateGovernor::setRenderState(RenderState state) {
    m_state = state;
    m_timer.stop();

    int interval = intervalMs();
    if (interval > 0) {
        m_timer.start(interval);
    }
}

void FramerateGovernor::onTimerTimeout() {
    emit renderTick();
}

} // namespace PocketPartner::DesktopCompanion
