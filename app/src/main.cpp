#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include "pocketpartner/emulator/NullEmulatorEngine.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocketpartner/core/CompanionLink.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PocketPartner");
    app.setOrganizationName("PocketPartnerProject");

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("PocketPartner - Desktop Companion & Emulator Host");
    mainWindow.resize(800, 600);

    QWidget *centralWidget = new QWidget(&mainWindow);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    QLabel *headerLabel = new QLabel("<h2>PocketPartner Emulator Host & Companion Manager</h2>", centralWidget);
    headerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(headerLabel);

    QLabel *statusLabel = new QLabel("Status: Ready. Load a ROM file to start.", centralWidget);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    QPushButton *loadRomButton = new QPushButton("Load ROM File", centralWidget);
    layout->addWidget(loadRomButton);

    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();

    return app.exec();
}
