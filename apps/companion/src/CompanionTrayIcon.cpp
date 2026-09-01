#include "CompanionTrayIcon.hpp"
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QProcess>
#include <QCoreApplication>

namespace Pocket::CompanionApp {

CompanionTrayIcon::CompanionTrayIcon(DesktopWidget *widget, std::shared_ptr<Core::IpcClient> ipcClient, QObject *parent)
    : QObject(parent), m_widget(widget), m_ipcClient(std::move(ipcClient)) {

    m_menu = new QMenu();

    m_openAppAction = m_menu->addAction("Open PocketPartner", this, &CompanionTrayIcon::onOpenMainApp);
    m_menu->addSeparator();

    m_toggleVisAction = m_menu->addAction("Hide Companion", this, &CompanionTrayIcon::onToggleVisibility);

    m_alwaysOnTopAction = m_menu->addAction("Always on Top");
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setChecked(m_widget->isAlwaysOnTop());
    connect(m_alwaysOnTopAction, &QAction::toggled, this, &CompanionTrayIcon::onToggleAlwaysOnTop);

    m_menu->addSeparator();
    m_exitAction = m_menu->addAction("Exit", this, &CompanionTrayIcon::onExit);

    // Render system tray icon programmatically (Asset policy compliance)
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(70, 160, 240));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 28, 28);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "P");

    m_trayIcon = new QSystemTrayIcon(QIcon(pixmap), this);
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->setToolTip("PocketPartner Companion");
}

void CompanionTrayIcon::show() {
    if (m_trayIcon) {
        m_trayIcon->show();
    }
}

void CompanionTrayIcon::onOpenMainApp() {
    if (m_ipcClient && m_ipcClient->isConnected()) {
        Core::IpcMessage msg;
        msg.command = Core::IpcCommandType::OpenMainApplication;
        m_ipcClient->sendMessage(msg);
    } else {
        // If PocketPartner.exe is not running, launch process executable directly
        QString mainAppPath = QCoreApplication::applicationDirPath() + "/PocketPartner.exe";
        QProcess::startDetached(mainAppPath, {});
    }
}

void CompanionTrayIcon::onToggleVisibility() {
    if (!m_widget) return;

    if (m_widget->isVisible()) {
        m_widget->hide();
        m_toggleVisAction->setText("Show Companion");
    } else {
        m_widget->show();
        m_toggleVisAction->setText("Hide Companion");
    }
}

void CompanionTrayIcon::onToggleAlwaysOnTop(bool checked) {
    if (m_widget) {
        m_widget->setAlwaysOnTop(checked);
    }
}

void CompanionTrayIcon::onExit() {
    QApplication::quit();
}

} // namespace Pocket::CompanionApp
