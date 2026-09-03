#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QElapsedTimer>
#include <QDebug>
#include "MainWindow.hpp"
#include "Theme.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include "pocket/storage/GameRepository.hpp"
#include "pocket/core/IpcServer.hpp"

int main(int argc, char *argv[]) {
    QElapsedTimer startupTimer;
    startupTimer.start();

    QApplication app(argc, argv);
    app.setApplicationName("PocketPartner");
    app.setOrganizationName("PocketPartnerProject");

    // Apply dark theme before window initialization
    Pocket::App::Theme::applyTheme(app);

    // Ensure AppData directory exists
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    QString dbPath = appDataDir + "/pocketpartner.db";

    // Initialize Database
    PocketPartner::Storage::DatabaseConfig dbConfig;
    dbConfig.dbPath = dbPath.toStdString();
    auto dbManager = std::make_shared<PocketPartner::Storage::DatabaseManager>(dbConfig);

    if (!dbManager->initialize()) {
        qCritical() << "Fatal: Failed to initialize SQLite database at" << dbPath;
        return 1;
    }

    // Run schema migrations
    QSqlDatabase db = QSqlDatabase::database();
    if (!Pocket::Storage::SchemaMigration::runMigrations(db)) {
        qCritical() << "Fatal: Failed to execute database migrations.";
        return 1;
    }

    // Initialize Game Repository
    auto gameRepo = std::make_shared<Pocket::Storage::GameRepository>(dbManager);

    // Initialize IPC Server
    auto ipcServer = std::make_shared<Pocket::Core::IpcServer>("PocketPartner_IPC_Pipe");
    if (!ipcServer->start()) {
        qWarning() << "Warning: Could not start IPC Server pipe.";
    }

    qint64 startupTimeMs = startupTimer.elapsed();
    qInfo() << "PocketPartner cold startup time:" << startupTimeMs << "ms";

    Pocket::App::MainWindow mainWindow(dbManager, gameRepo);
    mainWindow.show();

    // Listen for IPC messages from PocketCompanion.exe
    QObject::connect(ipcServer.get(), &Pocket::Core::IpcServer::messageReceived, [&mainWindow](QLocalSocket*, const Pocket::Core::IpcMessage& msg) {
        if (msg.command == Pocket::Core::IpcCommandType::OpenMainApplication) {
            mainWindow.showNormal();
            mainWindow.activateWindow();
            mainWindow.raise();
        }
    });

    return app.exec();
}
