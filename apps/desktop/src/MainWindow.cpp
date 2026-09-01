#include "MainWindow.hpp"
#include <QStatusBar>

namespace Pocket::App {

MainWindow::MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                       std::shared_ptr<Storage::GameRepository> gameRepo,
                       QWidget *parent)
    : QMainWindow(parent),
      m_dbManager(std::move(dbManager)),
      m_gameRepo(std::move(gameRepo)) {

    setWindowTitle("PocketPartner - Desktop Companion & Emulator Shell");
    resize(900, 650);

    m_tabWidget = new QTabWidget(this);

    m_libraryPage = new LibraryWidget(m_gameRepo, this);
    m_companionPage = new CompanionWidget(this);
    m_settingsPage = new SettingsWidget(m_dbManager, this);

    m_tabWidget->addTab(m_libraryPage, "Library");
    m_tabWidget->addTab(m_companionPage, "Companion");
    m_tabWidget->addTab(m_settingsPage, "Settings");

    setCentralWidget(m_tabWidget);
    statusBar()->showMessage("PocketPartner Ready.");
}

} // namespace Pocket::App
