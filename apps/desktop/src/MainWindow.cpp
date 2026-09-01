#include "MainWindow.hpp"
#include <QIcon>
#include <QPixmap>
#include <QPainter>

namespace Pocket::App {

MainWindow::MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                       std::shared_ptr<Pocket::Storage::GameRepository> gameRepo,
                       QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("PocketPartner - Desktop Companion & Emulator");
    resize(900, 600);

    m_tabWidget = new QTabWidget(this);

    m_libraryWidget = new LibraryWidget(gameRepo, m_tabWidget);
    m_companionWidget = new CompanionWidget(m_tabWidget);
    m_settingsWidget = new SettingsWidget(dbManager, m_tabWidget);
    m_diagnosticsWidget = new DiagnosticsWidget(m_tabWidget);

    m_tabWidget->addTab(m_libraryWidget, "Library");
    m_tabWidget->addTab(m_companionWidget, "Companion");
    m_tabWidget->addTab(m_settingsWidget, "Settings");
    m_tabWidget->addTab(m_diagnosticsWidget, "Diagnostics");

    setCentralWidget(m_tabWidget);
}

} // namespace Pocket::App
