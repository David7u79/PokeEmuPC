#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QMouseEvent>
#include "pocket/core/IpcClient.hpp"
#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

namespace Pocket::CompanionApp {

class DesktopWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWidget(std::shared_ptr<Core::IpcClient> ipcClient, QWidget *parent = nullptr);

    void setAlwaysOnTop(bool onTop);
    bool isAlwaysOnTop() const { return m_alwaysOnTop; }

    void updateStatus(double hunger, double mood, int level);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    std::shared_ptr<Core::IpcClient> m_ipcClient;
    QPoint m_dragPosition;
    bool m_alwaysOnTop{true};

    QLabel *m_nameLabel{nullptr};
    QLabel *m_levelLabel{nullptr};
    QProgressBar *m_hungerBar{nullptr};
    QProgressBar *m_moodBar{nullptr};

    PocketPartner::DesktopCompanion::FramerateGovernor m_governor;
};

} // namespace Pocket::CompanionApp
