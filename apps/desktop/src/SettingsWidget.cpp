#include "SettingsWidget.hpp"
#include "DiagnosticsWidget.hpp"
#include "Theme.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QComboBox>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QSqlDatabase>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Pocket::App {

namespace {

QLabel* createBlockTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text.toUpper(), parent);
    label->setStyleSheet(QStringLiteral(
        "font-size: 11px;"
        "font-weight: 700;"
        "letter-spacing: 1px;"
        "color: %1;"
        "background: transparent;"
        "padding-bottom: 2px;"
    ).arg(Theme::textSecondary().name()));
    return label;
}

QLabel* createElidedPathLabel(const QString& fullPath, QWidget* parent)
{
    auto* label = new QLabel(parent);
    label->setToolTip(fullPath);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    const QFontMetrics fm(label->font());
    label->setText(fm.elidedText(fullPath, Qt::ElideMiddle, 380));
    label->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(Theme::textPrimary().name()));
    return label;
}

} // namespace

SettingsWidget::SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                               std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent)
    : QWidget(parent), m_db(std::move(dbManager))
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 16, 20, 16);
    outerLayout->setSpacing(14);

    // Page header
    auto* headerWidget = new QWidget(this);
    auto* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto* titleLabel = new QLabel(QStringLiteral("Ajustes"), headerWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(Theme::textPrimary().name()));

    auto* descLabel = new QLabel(QStringLiteral("Configuración general, mandos, núcleos de emulación y diagnóstico."), headerWidget);
    descLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(Theme::textSecondary().name()));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(descLabel);
    outerLayout->addWidget(headerWidget);

    // General page container
    auto* generalPage = new QWidget(this);
    auto* generalLayout = new QVBoxLayout(generalPage);
    generalLayout->setContentsMargins(16, 16, 16, 16);
    generalLayout->setSpacing(12);

    // Block 1: Emulator & Save File Source
    generalLayout->addWidget(createBlockTitle(QStringLiteral("Emulador y sincronización de partidas"), generalPage));

    auto* syncForm = new QFormLayout();
    syncForm->setLabelAlignment(Qt::AlignLeft);
    syncForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    syncForm->setHorizontalSpacing(16);
    syncForm->setVerticalSpacing(10);

    auto* sourceCombo = new QComboBox(generalPage);
    sourceCombo->addItem("Internal Emulator (mGBA Core)");
    sourceCombo->addItem("External Save File (.sav Watcher)");

    auto* syncModeLabel = new QLabel("External Save: Read-only synchronization active", generalPage);
    syncModeLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::textPrimary().name()));

    auto* safetyNote = new QLabel("Nota: La sincronización de solo lectura mantiene el estado del compañero actualizado sin riesgo de corromper la partida externa.", generalPage);
    safetyNote->setWordWrap(true);
    safetyNote->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; background: transparent;").arg(Theme::textSecondary().name()));

    auto* lblSource = new QLabel(QStringLiteral("Origen de partida:"), generalPage);
    lblSource->setFixedWidth(160);
    lblSource->setStyleSheet(QStringLiteral("color: %1; font-weight: 500;").arg(Theme::textSecondary().name()));

    auto* lblPolicy = new QLabel(QStringLiteral("Política de seguridad:"), generalPage);
    lblPolicy->setFixedWidth(160);
    lblPolicy->setStyleSheet(QStringLiteral("color: %1; font-weight: 500;").arg(Theme::textSecondary().name()));

    syncForm->addRow(lblSource, sourceCombo);
    syncForm->addRow(lblPolicy, syncModeLabel);
    syncForm->addRow(new QLabel(generalPage), safetyNote);
    generalLayout->addLayout(syncForm);

    generalLayout->addSpacing(8);

    // Block 2: Database Engine
    generalLayout->addWidget(createBlockTitle(QStringLiteral("Motor de base de datos"), generalPage));

    auto* dbForm = new QFormLayout();
    dbForm->setLabelAlignment(Qt::AlignLeft);
    dbForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    dbForm->setHorizontalSpacing(16);
    dbForm->setVerticalSpacing(10);

    QSqlDatabase db = QSqlDatabase::database();
    const int schemaVersion = Storage::SchemaMigration::getCurrentVersion(db);

    auto* lblSqlite = new QLabel(QStringLiteral("Ruta SQLite:"), generalPage);
    lblSqlite->setFixedWidth(160);
    lblSqlite->setStyleSheet(QStringLiteral("color: %1; font-weight: 500;").arg(Theme::textSecondary().name()));

    auto* lblSchema = new QLabel(QStringLiteral("Versión del esquema:"), generalPage);
    lblSchema->setFixedWidth(160);
    lblSchema->setStyleSheet(QStringLiteral("color: %1; font-weight: 500;").arg(Theme::textSecondary().name()));

    auto* lblFramework = new QLabel(QStringLiteral("Entorno de interfaz:"), generalPage);
    lblFramework->setFixedWidth(160);
    lblFramework->setStyleSheet(QStringLiteral("color: %1; font-weight: 500;").arg(Theme::textSecondary().name()));

    auto* schemaVal = new QLabel(QString("v%1 (Actualizado)").arg(schemaVersion), generalPage);
    schemaVal->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::textPrimary().name()));

    auto* frameworkVal = new QLabel("Qt Widgets (MSVC 2022 64-bit)", generalPage);
    frameworkVal->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::textPrimary().name()));

    dbForm->addRow(lblSqlite, createElidedPathLabel(db.databaseName(), generalPage));
    dbForm->addRow(lblSchema, schemaVal);
    dbForm->addRow(lblFramework, frameworkVal);
    generalLayout->addLayout(dbForm);

    generalLayout->addStretch();

    // Internal sub-widgets
    m_coresWidget = new CoresWidget(this);
    connect(m_coresWidget, &CoresWidget::corePathChanged, this, [this](Core::GameSystem system, const QString& path) {
        if (system == Core::GameSystem::NDS)
            emit melonDsCorePathChanged(path);
        else
            emit coreLibraryPathChanged(path);
    });
    m_controllerMapper = new ControllerMapperWidget(std::move(mapping), this);
    m_diagnosticsWidget = new DiagnosticsWidget(this);

    // Tab sections
    auto* tabs = new QTabWidget(this);
    tabs->setObjectName("settingsTabs");
    tabs->setStyleSheet(QStringLiteral(
        "QTabWidget#settingsTabs::pane {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  margin-top: -1px;"
        "}"
        "QTabWidget#settingsTabs QTabBar::tab {"
        "  background-color: %3;"
        "  color: %4;"
        "  border: 1px solid %2;"
        "  border-bottom: none;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  padding: 7px 18px;"
        "  margin-right: 4px;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "}"
        "QTabWidget#settingsTabs QTabBar::tab:selected {"
        "  background-color: %1;"
        "  color: %5;"
        "  border-bottom: 2px solid %6;"
        "}"
        "QTabWidget#settingsTabs QTabBar::tab:hover:!selected {"
        "  background-color: %7;"
        "  color: %5;"
        "}"
    ).arg(Theme::surface().name(),
         Theme::border().name(),
         Theme::surfaceRaised().name(),
         Theme::textSecondary().name(),
         Theme::textPrimary().name(),
         Theme::accent().name(),
         Theme::border().name()));

    tabs->addTab(generalPage, QStringLiteral("General"));
    tabs->addTab(m_controllerMapper, QStringLiteral("Controles"));
    tabs->addTab(m_coresWidget, QStringLiteral("Cores"));
    tabs->addTab(m_diagnosticsWidget, QStringLiteral("Diagnóstico"));

    outerLayout->addWidget(tabs);
}

} // namespace Pocket::App
