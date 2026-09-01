#pragma once

#include <QObject>
#include <QTimer>

namespace Pocket::CompanionApp {

enum class AnimationState {
    Hidden,
    Static,
    SlowIdle,
    Interactive
};

class CompanionAnimationController : public QObject {
    Q_OBJECT
public:
    explicit CompanionAnimationController(QObject *parent = nullptr);
    ~CompanionAnimationController() override = default;

    AnimationState currentState() const { return m_currentState; }
    int targetFps() const { return m_targetFps; }
    bool isBatterySaverEnforced() const { return m_batterySaverEnforced; }

    void setState(AnimationState state);
    void triggerInteraction(int durationMs = 1500);
    void updatePowerPolicy(bool isBatterySaverActive);

signals:
    void frameTick();
    void stateChanged(AnimationState newState);

private slots:
    void onTimerTimeout();
    void onInteractionExpired();

private:
    void applyState(AnimationState state);

    AnimationState m_currentState{AnimationState::Static};
    AnimationState m_baseState{AnimationState::Static};
    int m_targetFps{0};
    bool m_batterySaverEnforced{false};

    QTimer m_animTimer;
    QTimer m_interactionTimer;
};

} // namespace Pocket::CompanionApp
