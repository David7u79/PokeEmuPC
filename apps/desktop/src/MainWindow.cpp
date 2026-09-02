#include "MainWindow.hpp"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSettings>

namespace Pocket::App {

MainWindow::MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                       std::shared_ptr<Pocket::Storage::GameRepository> gameRepo,
                       QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("PocketPartner - Desktop Companion & Emulator");
    resize(900, 600);

    m_tabWidget = new QTabWidget(this);

    // One mapping shared by the configuration UI and the running emulator.
    m_controllerMapping = std::make_shared<Pocket::Input::ControllerMapping>();
    {
        QSettings settings("PocketPartnerProject", "PocketPartner");
        if (!m_controllerMapping->load(settings)) {
            *m_controllerMapping = Pocket::Input::ControllerMapping::keyboardPreset();
        }
    }

    m_libraryWidget = new LibraryWidget(gameRepo, m_tabWidget);
    m_emulatorWidget = new EmulatorWidget(m_tabWidget);
    m_emulatorWidget->setControllerMapping(m_controllerMapping);
    m_companionWidget = new CompanionWidget(m_tabWidget);
    m_settingsWidget = new SettingsWidget(dbManager, m_controllerMapping, m_tabWidget);
    m_diagnosticsWidget = new DiagnosticsWidget(m_tabWidget);

    m_tabWidget->addTab(m_libraryWidget, "Library");
    m_tabWidget->addTab(m_emulatorWidget, "Emulator");
    m_tabWidget->addTab(m_companionWidget, "Companion");
    m_tabWidget->addTab(m_settingsWidget, "Settings");
    m_tabWidget->addTab(m_diagnosticsWidget, "Diagnostics");

    setCentralWidget(m_tabWidget);

    connect(m_libraryWidget, &LibraryWidget::gameSelected, this, [this](const Core::Game& game) {
        if (game.system == Core::GameSystem::GB || game.system == Core::GameSystem::GBC || game.system == Core::GameSystem::GBA) {
            QSettings settings("PocketPartnerProject", "PocketPartner");
            m_emulatorWidget->setCoreLibraryPath(settings.value("emulator/mgbaCorePath").toString());
            m_emulatorWidget->setControllerSystem(QString::fromStdString(Core::GameSystemUtils::toString(game.system)));
            m_emulatorWidget->loadAndStartRom(QString::fromStdString(game.romPath));
            m_tabWidget->setCurrentWidget(m_emulatorWidget);
        } else {
            m_emulatorWidget->setStatusMessage("system not supported by the internal core");
        }
    });

    // Rebinding a control in Settings takes effect without restarting the game.
    connect(m_settingsWidget->controllerMapper(), &ControllerMapperWidget::mappingChanged,
            m_emulatorWidget, &EmulatorWidget::refreshKeyBindings);
    connect(m_settingsWidget, &SettingsWidget::coreLibraryPathChanged, m_emulatorWidget, &EmulatorWidget::setCoreLibraryPath);
}

} // namespace Pocket::App
