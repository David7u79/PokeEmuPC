#include <QApplication>
#include <memory>
#include "DesktopWidget.hpp"
#include "CompanionTrayIcon.hpp"
#include "pocket/core/IpcClient.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PocketCompanion");
    app.setOrganizationName("PocketPartnerProject");
    app.setQuitOnLastWindowClosed(false); // Closing overlay window keeps system tray icon running

    // Initialize IPC Client
    auto ipcClient = std::make_shared<Pocket::Core::IpcClient>("PocketPartnerIPC");
    ipcClient->connectToServer();

    // Initialize Companion Overlay Widget & System Tray Icon
    auto *widget = new Pocket::Companion::DesktopWidget();
    widget->show();

    auto *trayIcon = new Pocket::CompanionApp::CompanionTrayIcon(widget, ipcClient);
    trayIcon->show();

    // Listen for IPC commands from PocketPartner.exe
    QObject::connect(ipcClient.get(), &Pocket::Core::IpcClient::messageReceived, [widget, &app](const Pocket::Core::IpcMessage& msg) {
        if (msg.command == Pocket::Core::IpcCommandType::ShowCompanion) {
            widget->show();
        } else if (msg.command == Pocket::Core::IpcCommandType::HideCompanion) {
            widget->hide();
        } else if (msg.command == Pocket::Core::IpcCommandType::ShutdownCompanion) {
            app.quit();
        }
    });

    return app.exec();
}
