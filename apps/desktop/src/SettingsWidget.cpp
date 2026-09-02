#include "SettingsWidget.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSqlDatabase>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Pocket::App {

SettingsWidget::SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                               std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent)
    : QWidget(parent), m_db(std::move(dbManager))
{
    auto* generalPage = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(generalPage);
    mainLayout->addWidget(new QLabel("<h2>Application Settings & Synchronization</h2>", generalPage));

    auto* syncGroup = new QGroupBox("Emulator & Save File Source Settings", generalPage);
    auto* syncForm = new QFormLayout(syncGroup);
    auto* sourceCombo = new QComboBox(syncGroup);
    sourceCombo->addItem("Internal Emulator (mGBA Core)");
    sourceCombo->addItem("External Save File (.sav Watcher)");
    auto* syncModeLabel = new QLabel("External Save: Read-only synchronization active", syncGroup);
    syncModeLabel->setStyleSheet("font-weight: bold; color: #88C0D0;");
    auto* safetyNote = new QLabel("Note: Read-only synchronization keeps desktop companion status updated without "
                                  "risking external save file corruption.",
                                  syncGroup);
    safetyNote->setWordWrap(true);
    safetyNote->setStyleSheet("font-size: 11px; color: #D8DEE9;");
    syncForm->addRow("Default Game Source:", sourceCombo);
    syncForm->addRow("Sync Safety Policy:", syncModeLabel);
    syncForm->addRow("", safetyNote);
    mainLayout->addWidget(syncGroup);

    auto* dbGroup = new QGroupBox("Database Engine", generalPage);
    auto* formLayout = new QFormLayout(dbGroup);
    QSqlDatabase db = QSqlDatabase::database();
    const int schemaVersion = Storage::SchemaMigration::getCurrentVersion(db);
    formLayout->addRow("SQLite Path:", new QLabel(db.databaseName(), dbGroup));
    formLayout->addRow("Schema Version:", new QLabel(QString("v%1 (Up to date)").arg(schemaVersion), dbGroup));
    formLayout->addRow("UI Framework:", new QLabel("Qt Widgets (MSVC 2022 64-bit)", dbGroup));
    mainLayout->addWidget(dbGroup);
    mainLayout->addStretch();

    m_coresWidget = new CoresWidget(this);
    connect(m_coresWidget, &CoresWidget::corePathChanged, this, [this](Core::GameSystem system, const QString& path) {
        if (system == Core::GameSystem::NDS)
            emit melonDsCorePathChanged(path);
        else
            emit coreLibraryPathChanged(path);
    });
    m_controllerMapper = new ControllerMapperWidget(std::move(mapping), this);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(generalPage, "General");
    tabs->addTab(m_coresWidget, "Cores");
    tabs->addTab(m_controllerMapper, "Controls");

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(tabs);
}

} // namespace Pocket::App
