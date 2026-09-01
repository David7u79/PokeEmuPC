#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPoint>
#include <QProgressBar>
#include <memory>
#include "pocket/companion/CompanionState.hpp"
#include "pocket/companion/CompanionSimulator.hpp"
#include "pocket/companion/IClock.hpp"
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/core/IpcClient.hpp"
#include "CompanionAnimationController.hpp"
#include "PowerStatusMonitor.hpp"

namespace Pocket::Companion {

class CompanionVisualCanvas : public QWidget {
    Q_OBJECT
public:
    explicit CompanionVisualCanvas(QWidget *parent = nullptr);
    void setAnimationFrame(int frame) { m_animFrame = frame; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_animFrame{0};
};

class DesktopWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget() override;

    void updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onFeedClicked();
    void onPetClicked();
    void onPlayClicked();
    void onRestClicked();
    void onTrainClicked();
    void onOpenGameClicked();
    void toggleExpandedView();
    void refreshUi();
    void onFrameTick();
    void onIpcMessageReceived(const Pocket::Core::IpcMessage& message);

private:
    void loadSavedPosition();
    void saveCurrentPosition();

    bool m_isDragging{false};
    QPoint m_dragPosition;
    bool m_isExpanded{false};
    int m_animStep{0};

    std::shared_ptr<SystemClock> m_clock;
    CompanionSimulator m_simulator;
    CompanionState m_state;

    Pocket::Core::IpcClient m_ipcClient;
    Pocket::CompanionApp::CompanionAnimationController m_animController;

    // Visual Canvas Placeholder (Zero proprietary sprites)
    CompanionVisualCanvas *m_canvas{nullptr};

    // Compact Display Labels & Progress Bars
    QLabel *m_nicknameLabel{nullptr};
    QLabel *m_canonicalMetaLabel{nullptr};
    QLabel *m_linkStatusLabel{nullptr};

    QProgressBar *m_bondBar{nullptr};
    QProgressBar *m_energyBar{nullptr};

    // App-Only Detailed Labels
    QLabel *m_hungerLabel{nullptr};
    QLabel *m_moodLabel{nullptr};
    QLabel *m_energyDetailLabel{nullptr};
    QLabel *m_bondDetailLabel{nullptr};

    // Action Buttons
    QPushButton *m_feedBtn{nullptr};
    QPushButton *m_petBtn{nullptr};
    QPushButton *m_playBtn{nullptr};
    QPushButton *m_restBtn{nullptr};
    QPushButton *m_trainBtn{nullptr};
    QPushButton *m_openGameBtn{nullptr};
    QPushButton *m_expandBtn{nullptr};

    QWidget *m_expandedContainer{nullptr};
    QTimer *m_decayTimer{nullptr};
    QTimer *m_powerCheckTimer{nullptr};
};

} // namespace Pocket::Companion
