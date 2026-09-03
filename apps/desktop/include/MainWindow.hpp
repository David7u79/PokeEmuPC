#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QPushButton>
#include <QSlider>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/GameRepository.hpp"
#include "LibraryWidget.hpp"
#include "CompanionWidget.hpp"
#include "SettingsWidget.hpp"
#include "EmulatorWidget.hpp"
#include "AudioSink.hpp"
#include "NdsDisplayWidget.hpp"
#include "AppNavigation.hpp"
#include "pocket/emulator/MelonDsEngine.hpp"

namespace Pocket::App {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
               std::shared_ptr<Pocket::Storage::GameRepository> gameRepo, QWidget* parent = nullptr);

    QStackedWidget* pages() const { return m_stackedWidget; }
    AppNavigation* navigation() const { return m_navigation; }
    LibraryWidget* libraryWidget() const { return m_libraryWidget; }
    CompanionWidget* companionWidget() const { return m_companionWidget; }
    SettingsWidget* settingsWidget() const { return m_settingsWidget; }
    QWidget* emulatorPage() const { return m_emulatorPage; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Flushes cartridge SRAM to disk before the core is torn down.
    void stopNdsEngine();
    Pocket::Emulator::LibretroEngineBase* activeEngine() const;
    QString activeSavePath() const;
    void saveActiveState(int slot);
    void loadActiveState(int slot);
    void updateEmulatorControls();

    AppNavigation* m_navigation{nullptr};
    QStackedWidget* m_stackedWidget{nullptr};
    QWidget* m_emulatorPage{nullptr};
    QPushButton* m_backToLibraryButton{nullptr};
    LibraryWidget* m_libraryWidget{nullptr};
    CompanionWidget* m_companionWidget{nullptr};
    SettingsWidget* m_settingsWidget{nullptr};
    EmulatorWidget* m_emulatorWidget{nullptr};
    QStackedWidget* m_emulatorStack{nullptr};
    QToolButton* m_speedButton{nullptr};
    QToolButton* m_saveStateButton{nullptr};
    QSlider* m_volumeSlider{nullptr};
    QToolButton* m_muteButton{nullptr};
    QTimer m_autoSaveTimer;
    int m_lastVolume{100};
    NdsDisplayWidget* m_ndsDisplayWidget{nullptr};
    std::unique_ptr<Pocket::Emulator::MelonDsEngine> m_ndsEngine;
    AudioSink m_ndsAudioSink;
    std::shared_ptr<Pocket::Input::ControllerMapping> m_controllerMapping;
};

} // namespace Pocket::App
