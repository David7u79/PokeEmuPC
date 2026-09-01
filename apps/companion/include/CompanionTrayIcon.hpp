#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <memory>
#include "DesktopWidget.hpp"
#include "CompanionStartupManager.hpp"
#include "pocket/core/IpcClient.hpp"

namespace Pocket::CompanionApp {

class CompanionTrayIcon : public QObject {
    Q_OBJECT
public:
    CompanionTrayIcon(Pocket::Companion::DesktopWidget *widget, std::shared_ptr<Core::IpcClient> ipcClient, QObject *parent = nullptr);
    ~CompanionTrayIcon() override = default;

    void show();

private slots:
    void onOpenMainApp();
    void onOpenGame();
    void onTrain();
    void onFeed();
    void onRest();
    void onStats();
    void onToggleVisibility();
    void onToggleAlwaysOnTop(bool checked);
    void onToggleAutostart(bool checked);
    void onExit();

private:
    Pocket::Companion::DesktopWidget *m_widget{nullptr};
    std::shared_ptr<Core::IpcClient> m_ipcClient;
    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_menu{nullptr};

    QAction *m_openAppAction{nullptr};
    QAction *m_openGameAction{nullptr};
    QAction *m_trainAction{nullptr};
    QAction *m_feedAction{nullptr};
    QAction *m_restAction{nullptr};
    QAction *m_statsAction{nullptr};
    QAction *m_toggleVisAction{nullptr};
    QAction *m_alwaysOnTopAction{nullptr};
    QAction *m_autostartAction{nullptr};
    QAction *m_exitAction{nullptr};
};

} // namespace Pocket::CompanionApp
