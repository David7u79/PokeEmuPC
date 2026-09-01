#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPoint>
#include <memory>
#include "pocket/companion/CompanionState.hpp"
#include "pocket/companion/CompanionSimulator.hpp"
#include "pocket/companion/IClock.hpp"
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/core/IpcClient.hpp"

namespace Pocket::Companion {

class DesktopWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget() override = default;

    void updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onFeedClicked();
    void onPetClicked();
    void onPlayClicked();
    void onRestClicked();
    void toggleExpandedView();
    void refreshUi();
    void onIpcMessageReceived(const Pocket::Core::IpcMessage& message);

private:
    bool m_isDragging{false};
    QPoint m_dragPosition;
    bool m_isExpanded{false};

    std::shared_ptr<SystemClock> m_clock;
    CompanionSimulator m_simulator;
    CompanionState m_state;

    Pocket::Core::IpcClient m_ipcClient;

    // Canonical Game State Labels
    QLabel *m_nicknameLabel{nullptr};
    QLabel *m_canonicalMetaLabel{nullptr};
    QLabel *m_linkStatusLabel{nullptr};

    // App-Only State Labels
    QLabel *m_hungerLabel{nullptr};
    QLabel *m_moodLabel{nullptr};
    QLabel *m_energyLabel{nullptr};
    QLabel *m_bondLabel{nullptr};

    // Action Buttons
    QPushButton *m_feedBtn{nullptr};
    QPushButton *m_petBtn{nullptr};
    QPushButton *m_playBtn{nullptr};
    QPushButton *m_restBtn{nullptr};
    QPushButton *m_expandBtn{nullptr};

    QWidget *m_expandedContainer{nullptr};
    QTimer *m_uiTimer{nullptr};
};

} // namespace Pocket::Companion
