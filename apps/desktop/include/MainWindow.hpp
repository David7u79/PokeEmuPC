#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/GameRepository.hpp"
#include "LibraryWidget.hpp"
#include "CompanionWidget.hpp"
#include "SettingsWidget.hpp"
#include "DiagnosticsWidget.hpp"
#include "EmulatorWidget.hpp"

namespace Pocket::App {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
               std::shared_ptr<Pocket::Storage::GameRepository> gameRepo,
               QWidget *parent = nullptr);

private:
    QTabWidget *m_tabWidget{nullptr};
    LibraryWidget *m_libraryWidget{nullptr};
    CompanionWidget *m_companionWidget{nullptr};
    SettingsWidget *m_settingsWidget{nullptr};
    DiagnosticsWidget *m_diagnosticsWidget{nullptr};
    EmulatorWidget *m_emulatorWidget{nullptr};
    std::shared_ptr<Pocket::Input::ControllerMapping> m_controllerMapping;
};

} // namespace Pocket::App
