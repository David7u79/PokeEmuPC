#include "CompanionAnimationController.hpp"

namespace Pocket::CompanionApp {

CompanionAnimationController::CompanionAnimationController(QObject *parent)
    : QObject(parent) {
    connect(&m_animTimer, &QTimer::timeout, this, &CompanionAnimationController::onTimerTimeout);
    connect(&m_interactionTimer, &QTimer::timeout, this, &CompanionAnimationController::onInteractionExpired);
    m_interactionTimer.setSingleShot(true);
}

void CompanionAnimationController::setState(AnimationState state) {
    m_baseState = state;
    if (m_currentState != AnimationState::Interactive) {
        applyState(state);
    }
}

void CompanionAnimationController::triggerInteraction(int durationMs) {
    if (m_batterySaverEnforced) return; // Ignore interaction boost on battery saver

    applyState(AnimationState::Interactive);
    m_interactionTimer.start(durationMs);
}

void CompanionAnimationController::updatePowerPolicy(bool isBatterySaverActive) {
    m_batterySaverEnforced = isBatterySaverActive;
    if (m_batterySaverEnforced) {
        m_interactionTimer.stop();
        applyState(AnimationState::Static);
    } else {
        applyState(m_baseState);
    }
}

void CompanionAnimationController::applyState(AnimationState state) {
    m_currentState = state;

    if (m_batterySaverEnforced && state != AnimationState::Hidden) {
        m_currentState = AnimationState::Static;
    }

    switch (m_currentState) {
        case AnimationState::Hidden:
        case AnimationState::Static:
            m_targetFps = 0;
            m_animTimer.stop();
            break;
        case AnimationState::SlowIdle:
            m_targetFps = 6;
            m_animTimer.start(166); // ~6 FPS
            break;
        case AnimationState::Interactive:
            m_targetFps = 25;
            m_animTimer.start(40); // 25 FPS
            break;
    }

    emit stateChanged(m_currentState);
}

void CompanionAnimationController::onTimerTimeout() {
    emit frameTick();
}

void CompanionAnimationController::onInteractionExpired() {
    applyState(m_baseState);
}

} // namespace Pocket::CompanionApp
