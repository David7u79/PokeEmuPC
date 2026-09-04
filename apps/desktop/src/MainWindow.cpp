#include "MainWindow.hpp"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSettings>
#include <QFile>
#include <QHBoxLayout>
#include <QMenu>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include "SaveStateSlots.hpp"
#include "Theme.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

namespace Pocket::App {

MainWindow::MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                       std::shared_ptr<Pocket::Storage::GameRepository> gameRepo, QWidget* parent)
    : QMainWindow(parent) {

    setWindowTitle("PocketPartner - Desktop Companion & Emulator");
    resize(900, 600);

#ifdef Q_OS_WIN
    // Paint the native caption in the application background so the window chrome
    // stops reading as a separate strip. Native buttons and dragging untouched;
    // the attributes are simply ignored on builds that do not know them.
    {
        const HWND handle = reinterpret_cast<HWND>(winId());
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(handle, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &darkMode, sizeof(darkMode));
        const QColor caption = Theme::surface();
        COLORREF captionColor = RGB(caption.red(), caption.green(), caption.blue());
        DwmSetWindowAttribute(handle, 35 /* DWMWA_CAPTION_COLOR */, &captionColor, sizeof(captionColor));
        COLORREF borderColor = captionColor;
        DwmSetWindowAttribute(handle, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));
    }
#endif

    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_navigation = new AppNavigation(centralWidget);
    m_stackedWidget = new QStackedWidget(centralWidget);
    m_stackedWidget->setObjectName("mainPages");

    centralLayout->addWidget(m_navigation);
    centralLayout->addWidget(m_stackedWidget);
    setCentralWidget(centralWidget);

    // One mapping shared by the configuration UI and the running emulator.
    m_controllerMapping = std::make_shared<Pocket::Input::ControllerMapping>();
    {
        QSettings settings("PocketPartnerProject", "PocketPartner");
        if (!m_controllerMapping->load(settings)) {
            *m_controllerMapping = Pocket::Input::ControllerMapping::keyboardPreset();
        }
    }

    m_gamepad = new GamepadReader(this);
    connect(m_gamepad, &GamepadReader::buttonChanged, this, &MainWindow::applyGamepadButton);

    m_libraryWidget = new LibraryWidget(gameRepo, m_stackedWidget);
    m_emulatorPage = new QWidget(m_stackedWidget);
    auto* emulatorLayout = new QVBoxLayout(m_emulatorPage);
    // Every pixel the layout keeps is a pixel of black around the console.
    emulatorLayout->setContentsMargins(0, 0, 0, 0);
    emulatorLayout->setSpacing(0);
    auto* controlsLayout = new QHBoxLayout;
    controlsLayout->setContentsMargins(6, 3, 6, 3);
    m_emulatorStack = new QStackedWidget(m_emulatorPage);
    m_emulatorWidget = new EmulatorWidget(m_emulatorStack);
    m_emulatorWidget->setControllerMapping(m_controllerMapping);
    m_ndsDisplayWidget = new NdsDisplayWidget(m_emulatorStack);
    m_ndsDisplayWidget->setControllerMapping(m_controllerMapping);
    m_emulatorStack->addWidget(m_emulatorWidget);
    m_emulatorStack->addWidget(m_ndsDisplayWidget);

    m_backToLibraryButton = new QPushButton(QString::fromUtf8("‹ Biblioteca"), m_emulatorPage);
    m_backToLibraryButton->setObjectName("backToLibraryButton");
    connect(m_backToLibraryButton, &QPushButton::clicked, this, [this] {
        m_stackedWidget->setCurrentWidget(m_libraryWidget);
        m_navigation->setActiveSection(AppNavigation::Section::Library);
    });

    m_speedButton = new QToolButton(m_emulatorPage);
    m_speedButton->setText("x1");
    m_speedButton->setPopupMode(QToolButton::InstantPopup);
    auto* speedMenu = new QMenu(m_speedButton);
    for (int speed = 1; speed <= 5; ++speed) {
        const QString label = speed == 1 ? QStringLiteral("Normal (x1)") : QStringLiteral("x%1").arg(speed);
        auto* action = speedMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, speed] {
            if (auto* engine = activeEngine()) {
                engine->setSpeedMultiplier(speed);
                m_speedButton->setText(QStringLiteral("x%1").arg(speed));
            }
        });
    }
    m_speedButton->setMenu(speedMenu);
    m_saveStateButton = new QToolButton(m_emulatorPage);
    m_saveStateButton->setText("Save state");
    m_saveStateButton->setPopupMode(QToolButton::InstantPopup);
    auto* statesMenu = new QMenu(m_saveStateButton);
    for (int slot = 1; slot <= kSaveStateSlotCount; ++slot) {
        auto* action = statesMenu->addAction(QStringLiteral("Guardar en slot %1").arg(slot));
        connect(action, &QAction::triggered, this, [this, slot] { saveActiveState(slot); });
    }
    statesMenu->addSeparator();
    for (int slot = 1; slot <= kSaveStateSlotCount; ++slot) {
        auto* action = statesMenu->addAction(QStringLiteral("Cargar slot %1").arg(slot));
        connect(action, &QAction::triggered, this, [this, slot] { loadActiveState(slot); });
    }
    statesMenu->addSeparator();
    auto* autoLoad = statesMenu->addAction("Cargar autoguardado");
    connect(autoLoad, &QAction::triggered, this, [this] { loadActiveState(0); });
    m_saveStateButton->setMenu(statesMenu);
    m_volumeSlider = new QSlider(Qt::Horizontal, m_emulatorPage);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setFixedWidth(120);
    QSettings volumeSettings("PocketPartnerProject", "PocketPartner");
    m_volumeSlider->setValue(std::clamp(volumeSettings.value("emulator/volume", 100).toInt(), 0, 100));
    m_lastVolume = m_volumeSlider->value() > 0 ? m_volumeSlider->value() : 100;
    m_muteButton = new QToolButton(m_emulatorPage);
    m_muteButton->setText("Mute");
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (value > 0)
            m_lastVolume = value;
        QSettings("PocketPartnerProject", "PocketPartner").setValue("emulator/volume", value);
        const float volume = value / 100.0f;
        m_emulatorWidget->audioSink().setVolume(volume);
        m_ndsAudioSink.setVolume(volume);
    });
    connect(m_muteButton, &QToolButton::clicked, this, [this] {
        m_volumeSlider->setValue(m_volumeSlider->value() == 0 ? m_lastVolume : 0);
    });

    controlsLayout->addWidget(m_backToLibraryButton);
    controlsLayout->addSpacing(8);
    controlsLayout->addWidget(m_speedButton);
    controlsLayout->addWidget(m_saveStateButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_volumeSlider);
    controlsLayout->addWidget(m_muteButton);
    emulatorLayout->addLayout(controlsLayout);
    emulatorLayout->addWidget(m_emulatorStack);

    m_companionWidget = new CompanionWidget(m_stackedWidget);
    m_settingsWidget = new SettingsWidget(dbManager, m_controllerMapping, m_stackedWidget);

    m_stackedWidget->addWidget(m_libraryWidget);
    m_stackedWidget->addWidget(m_companionWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    m_stackedWidget->addWidget(m_emulatorPage);

    m_stackedWidget->setCurrentWidget(m_libraryWidget);
    m_navigation->setActiveSection(AppNavigation::Section::Library);

    connect(m_navigation, &AppNavigation::resumeRequested, this, [this] {
        m_stackedWidget->setCurrentWidget(m_emulatorPage);
        m_emulatorStack->currentWidget()->setFocus();
    });

    connect(m_navigation, &AppNavigation::sectionSelected, this, [this](AppNavigation::Section section) {
        switch (section) {
        case AppNavigation::Section::Library:
            m_stackedWidget->setCurrentWidget(m_libraryWidget);
            break;
        case AppNavigation::Section::Companion:
            m_stackedWidget->setCurrentWidget(m_companionWidget);
            break;
        case AppNavigation::Section::Settings:
            m_stackedWidget->setCurrentWidget(m_settingsWidget);
            break;
        }
    });

    m_emulatorWidget->audioSink().setVolume(m_volumeSlider->value() / 100.0f);
    m_ndsAudioSink.setVolume(m_volumeSlider->value() / 100.0f);
    m_autoSaveTimer.setInterval(60000);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, [this] {
        if (auto* engine = activeEngine(); engine && engine->isRunning())
            saveActiveState(0);
        else
            m_autoSaveTimer.stop();
    });

    connect(m_libraryWidget, &LibraryWidget::gameSelected, this, [this](const Core::Game& game) {
        saveActiveState(0);
        m_autoSaveTimer.stop();
        if (game.system == Core::GameSystem::GB || game.system == Core::GameSystem::GBC ||
            game.system == Core::GameSystem::GBA) {
            stopNdsEngine();
            QSettings settings("PocketPartnerProject", "PocketPartner");
            m_emulatorWidget->setCoreLibraryPath(settings.value("emulator/mgbaCorePath").toString());
            m_emulatorWidget->setControllerSystem(QString::fromStdString(Core::GameSystemUtils::toString(game.system)));
            m_emulatorWidget->loadAndStartRom(QString::fromStdString(game.romPath));
            m_emulatorStack->setCurrentWidget(m_emulatorWidget);
            m_stackedWidget->setCurrentWidget(m_emulatorPage);
            if (m_emulatorWidget->engine() && m_emulatorWidget->engine()->isRunning())
                m_autoSaveTimer.start();
        } else if (game.system == Core::GameSystem::NDS) {
            m_emulatorWidget->stopEmulator();
            stopNdsEngine();
            QSettings settings("PocketPartnerProject", "PocketPartner");
            m_ndsEngine = std::make_unique<Pocket::Emulator::MelonDsEngine>(
                settings.value("emulator/melonDsCorePath").toString().toStdString());
            if (!m_ndsEngine->hasCore()) {
                m_emulatorWidget->setStatusMessage(QString::fromStdString(m_ndsEngine->coreError()));
                m_ndsEngine.reset();
                m_emulatorStack->setCurrentWidget(m_emulatorWidget);
                m_stackedWidget->setCurrentWidget(m_emulatorPage);
                updateEmulatorControls();
                return;
            }
            if (!m_ndsEngine->loadRom(game.romPath)) {
                m_emulatorWidget->setStatusMessage("Failed to load Nintendo DS ROM");
                m_ndsEngine.reset();
                m_emulatorStack->setCurrentWidget(m_emulatorWidget);
                m_stackedWidget->setCurrentWidget(m_emulatorPage);
                updateEmulatorControls();
                return;
            }
            // Only after loadRom: the core exposes no save RAM until a game is loaded.
            Pocket::Emulator::PersistentGameSave existingSave;
            if (existingSave.loadFromFile(m_ndsEngine->saveFilePath())) {
                m_ndsEngine->loadPersistentSave(existingSave);
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
            m_stackedWidget->setCurrentWidget(m_emulatorPage);
            m_autoSaveTimer.start();
        } else {
            m_emulatorWidget->setStatusMessage("system not supported by the internal core");
        }
        // Leaving the emulator was one click and coming back was none: the running
        // game gets a way back in the navigation bar for as long as it is loaded.
        if (activeEngine())
            m_navigation->setRunningGame(QString::fromStdString(game.title));
        else
            m_navigation->clearRunningGame();
        updateEmulatorControls();
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

void MainWindow::applyGamepadButton(int presetIndex, bool pressed) {
    const QStringList& ids = Pocket::Input::ControllerMapping::presetControlIds();
    if (presetIndex < 0 || presetIndex >= ids.size())
        return;

    // Remapping a control from the console art accepts pad buttons too, so the
    // press lands on the capture instead of on the game.
    const bool onEmulatorPage = m_stackedWidget->currentWidget() == m_emulatorPage;
    const bool gbaVisible = m_emulatorStack->currentWidget() == m_emulatorWidget;
    if (onEmulatorPage && gbaVisible && !m_emulatorWidget->capturingControlId().isEmpty()) {
        if (pressed) m_emulatorWidget->applyCapturedGamepadBinding(presetIndex);
        return;
    }

    auto* engine = activeEngine();
    if (!engine || !engine->isRunning() || !onEmulatorPage)
        return;

    const QString system = m_emulatorStack->currentWidget() == m_ndsDisplayWidget
        ? QStringLiteral("NDS") : QStringLiteral("GBA");

    // A control the user mapped to this pad button wins; otherwise the generic
    // preset order applies, so a pad works the moment it is plugged in.
    QString controlId = ids.at(presetIndex);
    if (m_controllerMapping) {
        for (const QString& candidate : ids) {
            const auto binding = m_controllerMapping->binding(system, candidate);
            if (binding && binding->device == Pocket::Input::InputDevice::Gamepad
                && binding->code == presetIndex) {
                controlId = candidate;
                break;
            }
        }
    }

    if (const auto button = Pocket::Input::ControllerMapping::emulatorButtonFor(controlId))
        engine->sendButtonEvent(*button, pressed);

    // Same feedback the keyboard gets: the console art lights the pressed control.
    if (m_emulatorStack->currentWidget() == m_ndsDisplayWidget)
        m_ndsDisplayWidget->setControlPressed(controlId, pressed);
    else
        m_emulatorWidget->setControlPressed(controlId, pressed);
}

Pocket::Emulator::LibretroEngineBase* MainWindow::activeEngine() const {
    if (m_emulatorStack->currentWidget() == m_ndsDisplayWidget)
        return m_ndsEngine.get();
    return m_emulatorWidget->engine();
}

QString MainWindow::activeSavePath() const {
    if (m_emulatorStack->currentWidget() == m_ndsDisplayWidget && m_ndsEngine)
        return QString::fromStdString(m_ndsEngine->saveFilePath());
    return m_emulatorWidget->savePath();
}

void MainWindow::saveActiveState(int slot) {
    auto* engine = activeEngine();
    const QString path = saveStatePath(activeSavePath(), slot);
    if (!engine || path.isEmpty())
        return;
    const std::vector<uint8_t> state = engine->saveState();
    if (state.empty())
        return;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
        file.write(reinterpret_cast<const char*>(state.data()), static_cast<qint64>(state.size()));
}

void MainWindow::loadActiveState(int slot) {
    auto* engine = activeEngine();
    QFile file(saveStatePath(activeSavePath(), slot));
    if (!engine || !file.open(QIODevice::ReadOnly))
        return;
    const QByteArray bytes = file.readAll();
    if (!bytes.isEmpty())
        engine->loadState(std::vector<uint8_t>(bytes.begin(), bytes.end()));
}

void MainWindow::updateEmulatorControls() {
    auto* engine = activeEngine();
    m_speedButton->setEnabled(engine != nullptr);
    m_saveStateButton->setEnabled(engine && engine->supportsSaveStates());
    m_speedButton->setText(QStringLiteral("x%1").arg(engine ? engine->speedMultiplier() : 1));
}

void MainWindow::stopNdsEngine() {
    if (m_ndsEngine) {
        if (m_emulatorStack->currentWidget() == m_ndsDisplayWidget)
            saveActiveState(0);
        // Read the path before stop(), which clears it, and flush before the core
        // is destroyed: otherwise the whole play session is lost on exit.
        const std::string savePath = m_ndsEngine->saveFilePath();
        if (!savePath.empty()) {
            const Pocket::Emulator::PersistentGameSave save = m_ndsEngine->getPersistentSave();
            if (!save.isEmpty()) {
                save.saveToFile(savePath);
            }
        }
        m_ndsEngine->stop();
        m_ndsEngine.reset();
    }
    m_ndsAudioSink.close();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Closing the window with a game running must still persist it.
    saveActiveState(0);
    m_autoSaveTimer.stop();
    m_emulatorWidget->stopEmulator();
    stopNdsEngine();
    QMainWindow::closeEvent(event);
}

} // namespace Pocket::App
