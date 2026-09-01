#include "SettingsWidget.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSqlDatabase>
#include <QComboBox>

namespace Pocket::App {

SettingsWidget::SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager, QWidget *parent)
    : QWidget(parent), m_db(std::move(dbManager)) {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *headerLabel = new QLabel("<h2>Application Settings & Synchronization</h2>", this);
    mainLayout->addWidget(headerLabel);

    QGroupBox *syncGroup = new QGroupBox("Emulator & Save File Source Settings", this);
    QFormLayout *syncForm = new QFormLayout(syncGroup);

    QComboBox *sourceCombo = new QComboBox(syncGroup);
    sourceCombo->addItem("Internal Emulator (mGBA Core)");
    sourceCombo->addItem("External Save File (.sav Watcher)");

    QLabel *syncModeLabel = new QLabel("External Save: Read-only synchronization active", syncGroup);
    syncModeLabel->setStyleSheet("font-weight: bold; color: #88C0D0;");

    QLabel *safetyNote = new QLabel("Note: Read-only synchronization keeps desktop companion status updated without risking external save file corruption.", syncGroup);
    safetyNote->setWordWrap(true);
    safetyNote->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    syncForm->addRow("Default Game Source:", sourceCombo);
    syncForm->addRow("Sync Safety Policy:", syncModeLabel);
    syncForm->addRow("", safetyNote);

    mainLayout->addWidget(syncGroup);

    QGroupBox *dbGroup = new QGroupBox("Database Engine", this);
    QFormLayout *formLayout = new QFormLayout(dbGroup);

    QSqlDatabase db = QSqlDatabase::database();
    m_dbPathLabel = new QLabel(db.databaseName(), dbGroup);

    int schemaVer = Storage::SchemaMigration::getCurrentVersion(db);
    m_versionLabel = new QLabel(QString("v%1 (Up to date)").arg(schemaVer), dbGroup);

    formLayout->addRow("SQLite Path:", m_dbPathLabel);
    formLayout->addRow("Schema Version:", m_versionLabel);
    formLayout->addRow("UI Framework:", new QLabel("Qt Widgets (MSVC 2022 64-bit)", dbGroup));

    mainLayout->addWidget(dbGroup);
    mainLayout->addStretch();
}

} // namespace Pocket::App
