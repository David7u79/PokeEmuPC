#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/GameRepository.hpp"
#include "LibraryWidget.hpp"
#include "CompanionWidget.hpp"
#include "SettingsWidget.hpp"

namespace Pocket::App {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                        std::shared_ptr<Storage::GameRepository> gameRepo,
                        QWidget *parent = nullptr);

private:
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_dbManager;
    std::shared_ptr<Storage::GameRepository> m_gameRepo;

    QTabWidget *m_tabWidget{nullptr};
    LibraryWidget *m_libraryPage{nullptr};
    CompanionWidget *m_companionPage{nullptr};
    SettingsWidget *m_settingsPage{nullptr};
};

} // namespace Pocket::App
