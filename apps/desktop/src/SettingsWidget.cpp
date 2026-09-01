#include "SettingsWidget.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSqlDatabase>

namespace Pocket::App {

SettingsWidget::SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager, QWidget *parent)
    : QWidget(parent), m_db(std::move(dbManager)) {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *headerLabel = new QLabel("<h2>Application Settings & Diagnostics</h2>", this);
    mainLayout->addWidget(headerLabel);

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
