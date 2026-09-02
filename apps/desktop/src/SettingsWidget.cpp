#include "SettingsWidget.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSqlDatabase>
#include <QComboBox>
#include <QFileDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>

namespace Pocket::App {

SettingsWidget::SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                               std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent)
    : QWidget(parent), m_db(std::move(dbManager)) {

    QWidget* generalPage = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(generalPage);

    QLabel* headerLabel = new QLabel("<h2>Application Settings & Synchronization</h2>", this);
    mainLayout->addWidget(headerLabel);

    QGroupBox* syncGroup = new QGroupBox("Emulator & Save File Source Settings", this);
    QFormLayout* syncForm = new QFormLayout(syncGroup);

    QComboBox* sourceCombo = new QComboBox(syncGroup);
    sourceCombo->addItem("Internal Emulator (mGBA Core)");
    sourceCombo->addItem("External Save File (.sav Watcher)");

    QLabel* syncModeLabel = new QLabel("External Save: Read-only synchronization active", syncGroup);
    syncModeLabel->setStyleSheet("font-weight: bold; color: #88C0D0;");

    QLabel* safetyNote = new QLabel("Note: Read-only synchronization keeps desktop companion status updated without "
                                    "risking external save file corruption.",
                                    syncGroup);
    safetyNote->setWordWrap(true);
    safetyNote->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    syncForm->addRow("Default Game Source:", sourceCombo);
    syncForm->addRow("Sync Safety Policy:", syncModeLabel);
    syncForm->addRow("", safetyNote);

    m_corePathEdit = new QLineEdit(syncGroup);
    m_corePathEdit->setReadOnly(true);
    auto* browseButton = new QPushButton("Browse...", syncGroup);
    auto* corePathLayout = new QHBoxLayout();
    corePathLayout->addWidget(m_corePathEdit);
    corePathLayout->addWidget(browseButton);
    syncForm->addRow("mGBA libretro core:", corePathLayout);
    m_coreStatusLabel = new QLabel(syncGroup);
    syncForm->addRow("", m_coreStatusLabel);
    QSettings settings("PocketPartnerProject", "PocketPartner");
    m_corePathEdit->setText(settings.value("emulator/mgbaCorePath").toString());
    updateCoreStatus(m_corePathEdit->text());
    connect(browseButton, &QPushButton::clicked, this, &SettingsWidget::browseCoreLibrary);

    m_melonDsCorePathEdit = new QLineEdit(syncGroup);
    m_melonDsCorePathEdit->setReadOnly(true);
    auto* melonBrowseButton = new QPushButton("Browse...", syncGroup);
    auto* melonPathLayout = new QHBoxLayout();
    melonPathLayout->addWidget(m_melonDsCorePathEdit);
    melonPathLayout->addWidget(melonBrowseButton);
    syncForm->addRow("melonDS DS libretro core:", melonPathLayout);
    m_melonDsCoreStatusLabel = new QLabel(syncGroup);
    syncForm->addRow("", m_melonDsCoreStatusLabel);
    m_melonDsCorePathEdit->setText(settings.value("emulator/melonDsCorePath").toString());
    updateMelonDsCoreStatus(m_melonDsCorePathEdit->text());
    connect(melonBrowseButton, &QPushButton::clicked, this, &SettingsWidget::browseMelonDsCoreLibrary);

    mainLayout->addWidget(syncGroup);

    QGroupBox* dbGroup = new QGroupBox("Database Engine", this);
    QFormLayout* formLayout = new QFormLayout(dbGroup);

    QSqlDatabase db = QSqlDatabase::database();
    m_dbPathLabel = new QLabel(db.databaseName(), dbGroup);

    int schemaVer = Storage::SchemaMigration::getCurrentVersion(db);
    m_versionLabel = new QLabel(QString("v%1 (Up to date)").arg(schemaVer), dbGroup);

    formLayout->addRow("SQLite Path:", m_dbPathLabel);
    formLayout->addRow("Schema Version:", m_versionLabel);
    formLayout->addRow("UI Framework:", new QLabel("Qt Widgets (MSVC 2022 64-bit)", dbGroup));

    mainLayout->addWidget(dbGroup);
    mainLayout->addStretch();

    m_controllerMapper = new ControllerMapperWidget(std::move(mapping), this);

    QTabWidget* tabs = new QTabWidget(this);
    tabs->addTab(generalPage, "General");
    tabs->addTab(m_controllerMapper, "Controls");

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(tabs);
}

void SettingsWidget::browseMelonDsCoreLibrary() {
#ifdef Q_OS_WIN
    const QString filter = "Libretro core (*.dll);;All files (*.*)";
#else
    const QString filter = "Libretro core (*.so);;All files (*.*)";
#endif
    const QString path =
        QFileDialog::getOpenFileName(this, "Select melonDS libretro core", m_melonDsCorePathEdit->text(), filter);
    if (path.isEmpty())
        return;
    m_melonDsCorePathEdit->setText(path);
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/melonDsCorePath", path);
    updateMelonDsCoreStatus(path);
    emit melonDsCorePathChanged(path);
}

void SettingsWidget::browseCoreLibrary() {
#ifdef Q_OS_WIN
    const QString filter = "Libretro core (*.dll);;All files (*.*)";
#else
    const QString filter = "Libretro core (*.so);;All files (*.*)";
#endif
    const QString path =
        QFileDialog::getOpenFileName(this, "Select mGBA libretro core", m_corePathEdit->text(), filter);
    if (path.isEmpty())
        return;
    m_corePathEdit->setText(path);
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/mgbaCorePath", path);
    updateCoreStatus(path);
    emit coreLibraryPathChanged(path);
}

void SettingsWidget::updateMelonDsCoreStatus(const QString& path) {
    const bool found = !path.isEmpty() && QFile::exists(path);
    m_melonDsCoreStatusLabel->setText(found ? "Core found" : "Core not set - download melondsds_libretro.dll");
    m_melonDsCoreStatusLabel->setStyleSheet(found ? "color: green;" : "color: red;");
}

void SettingsWidget::updateCoreStatus(const QString& path) {
    const bool found = !path.isEmpty() && QFile::exists(path);
    m_coreStatusLabel->setText(found ? "Core found" : "Core not set — download mgba_libretro.dll");
    m_coreStatusLabel->setStyleSheet(found ? "color: green;" : "color: red;");
}

} // namespace Pocket::App
