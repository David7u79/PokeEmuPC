#include "MainWindow.hpp"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSettings>

namespace Pocket::App {

MainWindow::MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                       std::shared_ptr<Pocket::Storage::GameRepository> gameRepo, QWidget* parent)
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
    m_emulatorStack = new QStackedWidget(m_tabWidget);
    m_emulatorWidget = new EmulatorWidget(m_emulatorStack);
    m_emulatorWidget->setControllerMapping(m_controllerMapping);
    m_ndsDisplayWidget = new NdsDisplayWidget(m_emulatorStack);
    m_ndsDisplayWidget->setControllerMapping(m_controllerMapping);
    m_emulatorStack->addWidget(m_emulatorWidget);
    m_emulatorStack->addWidget(m_ndsDisplayWidget);
    m_companionWidget = new CompanionWidget(m_tabWidget);
    m_settingsWidget = new SettingsWidget(dbManager, m_controllerMapping, m_tabWidget);
    m_diagnosticsWidget = new DiagnosticsWidget(m_tabWidget);

    m_tabWidget->addTab(m_libraryWidget, "Library");
    m_tabWidget->addTab(m_emulatorStack, "Emulator");
    m_tabWidget->addTab(m_companionWidget, "Companion");
    m_tabWidget->addTab(m_settingsWidget, "Settings");
    m_tabWidget->addTab(m_diagnosticsWidget, "Diagnostics");

    setCentralWidget(m_tabWidget);

    connect(m_libraryWidget, &LibraryWidget::gameSelected, this, [this](const Core::Game& game) {
        if (game.system == Core::GameSystem::GB || game.system == Core::GameSystem::GBC ||
            game.system == Core::GameSystem::GBA) {
            if (m_ndsEngine) {
                m_ndsEngine->stop();
                m_ndsEngine.reset();
            }
            m_ndsAudioSink.close();
            QSettings settings("PocketPartnerProject", "PocketPartner");
            m_emulatorWidget->setCoreLibraryPath(settings.value("emulator/mgbaCorePath").toString());
            m_emulatorWidget->setControllerSystem(QString::fromStdString(Core::GameSystemUtils::toString(game.system)));
            m_emulatorWidget->loadAndStartRom(QString::fromStdString(game.romPath));
            m_emulatorStack->setCurrentWidget(m_emulatorWidget);
            m_tabWidget->setCurrentWidget(m_emulatorStack);
        } else if (game.system == Core::GameSystem::NDS) {
            m_emulatorWidget->stopEmulator();
            if (m_ndsEngine)
                m_ndsEngine->stop();
            m_ndsAudioSink.close();
            QSettings settings("PocketPartnerProject", "PocketPartner");
            m_ndsEngine = std::make_unique<Pocket::Emulator::MelonDsEngine>(
                settings.value("emulator/melonDsCorePath").toString().toStdString());
            if (!m_ndsEngine->hasCore()) {
                m_emulatorWidget->setStatusMessage(QString::fromStdString(m_ndsEngine->coreError()));
                m_ndsEngine.reset();
                m_emulatorStack->setCurrentWidget(m_emulatorWidget);
                m_tabWidget->setCurrentWidget(m_emulatorStack);
                return;
            }
            if (!m_ndsEngine->loadRom(game.romPath)) {
                m_emulatorWidget->setStatusMessage("Failed to load Nintendo DS ROM");
                m_ndsEngine.reset();
                m_emulatorStack->setCurrentWidget(m_emulatorWidget);
                m_tabWidget->setCurrentWidget(m_emulatorStack);
                return;
            }
            m_ndsEngine->setVideoFrameCallback([this](const uint8_t* pixels, int width, int height, size_t pitch) {
                m_ndsDisplayWidget->submitCombinedFrame(pixels, width, height, pitch);
            });
            m_ndsAudioSink.open(static_cast<int>(m_ndsEngine->sampleRate()));
            m_ndsEngine->setAudioSampleCallback(
                [this](const int16_t* samples, size_t frames) { m_ndsAudioSink.submit(samples, frames); });
            m_ndsEngine->start();
            m_emulatorStack->setCurrentWidget(m_ndsDisplayWidget);
            m_ndsDisplayWidget->setFocus();
            m_tabWidget->setCurrentWidget(m_emulatorStack);
        } else {
            m_emulatorWidget->setStatusMessage("system not supported by the internal core");
        }
    });

    // Rebinding a control in Settings takes effect without restarting the game.
    connect(m_settingsWidget->controllerMapper(), &ControllerMapperWidget::mappingChanged, m_emulatorWidget,
            &EmulatorWidget::refreshKeyBindings);
    connect(m_settingsWidget->controllerMapper(), &ControllerMapperWidget::mappingChanged, m_ndsDisplayWidget,
            &NdsDisplayWidget::refreshKeyBindings);
    connect(m_settingsWidget, &SettingsWidget::coreLibraryPathChanged, m_emulatorWidget,
            &EmulatorWidget::setCoreLibraryPath);
    connect(m_ndsDisplayWidget, &NdsDisplayWidget::touchInputChanged, this, [this](int x, int y, bool pressed) {
        if (m_ndsEngine)
            m_ndsEngine->sendTouchInput(x, y, pressed);
    });
    connect(m_ndsDisplayWidget, &NdsDisplayWidget::buttonInputChanged, this,
            [this](Pocket::Emulator::EmulatorButton button, bool pressed) {
                if (m_ndsEngine)
                    m_ndsEngine->sendButtonEvent(button, pressed);
            });
}

} // namespace Pocket::App
