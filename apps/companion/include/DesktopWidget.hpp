#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QMouseEvent>
#include <memory>
#include "pocket/core/IpcClient.hpp"
#include "pocket/companion/CompanionSimulator.hpp"
#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

namespace Pocket::CompanionApp {

class DesktopWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWidget(std::shared_ptr<Core::IpcClient> ipcClient, QWidget *parent = nullptr);

    void setAlwaysOnTop(bool onTop);
    bool isAlwaysOnTop() const { return m_alwaysOnTop; }

    void refreshStateDisplay();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onFeedClicked();
    void onPetClicked();
    void onPlayClicked();
    void onRestClicked();
    void onToggleDetails();

private:
    std::shared_ptr<Core::IpcClient> m_ipcClient;
    QPoint m_dragPosition;
    bool m_alwaysOnTop{true};
    bool m_detailsExpanded{false};

    Pocket::Companion::CompanionSimulator m_simulator;
    Pocket::Companion::CompanionState m_state;

    QLabel *m_nameLabel{nullptr};
    QLabel *m_levelLabel{nullptr};
    QLabel *m_bondLabel{nullptr};

    QProgressBar *m_hungerBar{nullptr};
    QProgressBar *m_moodBar{nullptr};
    QProgressBar *m_energyBar{nullptr};

    QWidget *m_buttonContainer{nullptr};
    QPushButton *m_feedBtn{nullptr};
    QPushButton *m_petBtn{nullptr};
    QPushButton *m_playBtn{nullptr};
    QPushButton *m_restBtn{nullptr};
    QPushButton *m_detailsBtn{nullptr};

    QLabel *m_detailsLabel{nullptr};

    PocketPartner::DesktopCompanion::FramerateGovernor m_governor;
};

} // namespace Pocket::CompanionApp
