#include "CoresWidget.hpp"
#include "Theme.hpp"
#include "pocket/emulator/LibretroCoreProbe.hpp"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace Pocket::App {
namespace {

QString& settingsOrganization()
{
    static QString organization = QStringLiteral("PocketPartnerProject");
    return organization;
}

QString& settingsApplication()
{
    static QString application = QStringLiteral("PocketPartner");
    return application;
}

QString coreFilter()
{
#ifdef Q_OS_WIN
    return "Libretro core (*.dll);;All files (*.*)";
#else
    return "Libretro core (*.so);;All files (*.*)";
#endif
}

} // namespace

CoresWidget::CoresWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* blockTitle = new QLabel(QStringLiteral("NÚCLEOS DE EMULACIÓN LIBRETRO"), this);
    blockTitle->setStyleSheet(QStringLiteral(
        "font-size: 11px;"
        "font-weight: 700;"
        "letter-spacing: 1px;"
        "color: %1;"
        "background: transparent;"
        "padding-bottom: 2px;"
    ).arg(Theme::textSecondary().name()));
    layout->addWidget(blockTitle);

    auto addRow = [this, layout](CoreRow& row, bool isLast) {
        auto* rowWidget = new QWidget(this);
        auto* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(4, 8, 4, 8);
        rowLayout->setSpacing(16);

        // System name & expected core (left column)
        auto* infoCol = new QWidget(rowWidget);
        infoCol->setFixedWidth(220);
        auto* infoLayout = new QVBoxLayout(infoCol);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(2);

        auto* name = new QLabel(row.displayName, infoCol);
        QFont nameFont = name->font();
        nameFont.setBold(true);
        name->setFont(nameFont);
        name->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(Theme::textPrimary().name()));

        auto* expected = new QLabel(QString("Core esperado: %1").arg(row.expectedCore), infoCol);
        expected->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; background: transparent;").arg(Theme::textSecondary().name()));

        infoLayout->addWidget(name);
        infoLayout->addWidget(expected);
        rowLayout->addWidget(infoCol);

        // Status & Path (center column)
        auto* statusCol = new QWidget(rowWidget);
        auto* statusLayout = new QVBoxLayout(statusCol);
        statusLayout->setContentsMargins(0, 0, 0, 0);
        statusLayout->setSpacing(4);

        row.statusLabel = new QLabel(statusCol);
        row.statusLabel->setWordWrap(true);

        row.pathEdit = new QLineEdit(statusCol);
        row.pathEdit->setReadOnly(true);
        row.pathEdit->setClearButtonEnabled(false);
        row.pathEdit->setToolTip(row.pathEdit->text());
        row.pathEdit->setStyleSheet(QStringLiteral(
            "QLineEdit {"
            "  background-color: %1;"
            "  color: %2;"
            "  border: 1px solid %3;"
            "  border-radius: 4px;"
            "  padding: 4px 8px;"
            "  font-size: 12px;"
            "}"
        ).arg(Theme::surfaceRaised().name(), Theme::textPrimary().name(), Theme::border().name()));

        statusLayout->addWidget(row.statusLabel);
        statusLayout->addWidget(row.pathEdit);
        rowLayout->addWidget(statusCol, 1);

        // Actions (right column, discrete buttons)
        auto* btnCol = new QWidget(rowWidget);
        auto* btnLayout = new QHBoxLayout(btnCol);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(6);

        auto* browseButton = new QPushButton(QStringLiteral("Browse..."), btnCol);
        auto* importButton = new QPushButton(QStringLiteral("Import..."), btnCol);
        auto* clearButton = new QPushButton(QStringLiteral("Clear"), btnCol);

        btnLayout->addWidget(browseButton);
        btnLayout->addWidget(importButton);
        btnLayout->addWidget(clearButton);
        rowLayout->addWidget(btnCol);

        layout->addWidget(rowWidget);

        if (!isLast) {
            auto* sep = new QFrame(this);
            sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet(QStringLiteral("background-color: %1; max-height: 1px; border: none;").arg(Theme::border().name()));
            layout->addWidget(sep);
        }

        connect(browseButton, &QPushButton::clicked, this, [this, &row] { browse(row); });
        connect(importButton, &QPushButton::clicked, this, [this, &row] { importCore(row); });
        connect(clearButton, &QPushButton::clicked, this, [this, &row] { clear(row); });
    };

    addRow(m_mgba, false);
    addRow(m_melonDs, true);
    layout->addStretch();
    refresh();
}

void CoresWidget::setSettingsScope(const QString& organization, const QString& application)
{
    settingsOrganization() = organization;
    settingsApplication() = application;
}

QSettings CoresWidget::openSettings()
{
    return QSettings(settingsOrganization(), settingsApplication());
}

void CoresWidget::refresh()
{
    QSettings settings = openSettings();
    for (CoreRow* row : {&m_mgba, &m_melonDs}) {
        row->pathEdit->setText(settings.value(row->settingsKey).toString());
        row->pathEdit->setToolTip(row->pathEdit->text());
        updateStatus(*row);
    }
}

QString CoresWidget::corePath(Core::GameSystem system) const
{
    const CoreRow* row = rowFor(system);
    return row ? row->pathEdit->text() : QString();
}

void CoresWidget::browse(CoreRow& row)
{
    const QString path = QFileDialog::getOpenFileName(this, "Select " + row.expectedCore + " libretro core",
                                                       row.pathEdit->text(), coreFilter());
    if (!path.isEmpty())
        validateAndSave(row, path);
}

void CoresWidget::importCore(CoreRow& row)
{
    const QString source = QFileDialog::getOpenFileName(this, "Import " + row.expectedCore + " libretro core",
                                                         row.pathEdit->text(), coreFilter());
    if (source.isEmpty())
        return;

    const auto description = Emulator::probeLibretroCore(source.toStdString());
    if (!description.valid) {
        QMessageBox::critical(this, "Invalid libretro core", QString::fromStdString(description.error));
        return;
    }
    if (!Emulator::coreSupportsSystem(description, row.system) &&
        QMessageBox::question(this, "Core support warning",
                              "This core does not report support for " + row.displayName + ". Continue?") !=
            QMessageBox::Yes)
        return;

    const QString coreDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cores";
    if (!QDir().mkpath(coreDirectory)) {
        QMessageBox::critical(this, "Import failed", "Could not create the application cores directory.");
        return;
    }
    const QString target = QDir(coreDirectory).filePath(QFileInfo(source).fileName());
    if (QFile::exists(target) &&
        QMessageBox::question(this, "Replace core?", "A core with this name already exists. Replace it?") !=
            QMessageBox::Yes)
        return;
    if (QFile::exists(target) && !QFile::remove(target)) {
        QMessageBox::critical(this, "Import failed", "Could not replace the existing core.");
        return;
    }
    if (!QFile::copy(source, target)) {
        QMessageBox::critical(this, "Import failed", "Could not copy the selected core.");
        return;
    }
    validateAndSave(row, target);
}

void CoresWidget::clear(CoreRow& row)
{
    QSettings settings = openSettings();
    settings.remove(row.settingsKey);
    row.pathEdit->clear();
    row.pathEdit->setToolTip({});
    updateStatus(row);
    emit corePathChanged(row.system, {});
}

bool CoresWidget::validateAndSave(CoreRow& row, const QString& path)
{
    const auto description = Emulator::probeLibretroCore(path.toStdString());
    if (!description.valid) {
        QMessageBox::critical(this, "Invalid libretro core", QString::fromStdString(description.error));
        return false;
    }
    if (!Emulator::coreSupportsSystem(description, row.system) &&
        QMessageBox::question(this, "Core support warning",
                              "This core does not report support for " + row.displayName + ". Continue?") !=
            QMessageBox::Yes)
        return false;

    QSettings settings = openSettings();
    settings.setValue(row.settingsKey, path);
    row.pathEdit->setText(path);
    row.pathEdit->setToolTip(path);
    updateStatus(row);
    emit corePathChanged(row.system, path);
    return true;
}

void CoresWidget::updateStatus(CoreRow& row)
{
    const QString path = row.pathEdit->text();
    if (path.isEmpty()) {
        row.statusLabel->setText(QStringLiteral("Not configured"));
        row.statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 500; font-size: 12px;").arg(Theme::textSecondary().name()));
        return;
    }
    if (!QFileInfo::exists(path)) {
        row.statusLabel->setText(QStringLiteral("No encontrado: el archivo no existe"));
        row.statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 500; font-size: 12px;").arg(Theme::textDisabled().name()));
        return;
    }

    const auto description = Emulator::probeLibretroCore(path.toStdString());
    if (!description.valid) {
        row.statusLabel->setText(QString("No válido: %1").arg(QString::fromStdString(description.error)));
        row.statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 500; font-size: 12px;").arg(Theme::textDisabled().name()));
    } else if (!Emulator::coreSupportsSystem(description, row.system)) {
        row.statusLabel->setText(QString("Incompatible: no reporta soporte para %1").arg(row.displayName));
        row.statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 500; font-size: 12px;").arg(Theme::textSecondary().name()));
    } else {
        const QString coreInfo = QString::fromStdString(description.libraryName + " " + description.libraryVersion).trimmed();
        row.statusLabel->setText(QString("Presente: %1").arg(coreInfo));
        row.statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 600; font-size: 12px;").arg(Theme::accent().name()));
    }
}

CoresWidget::CoreRow* CoresWidget::rowFor(Core::GameSystem system)
{
    if (system == Core::GameSystem::GB || system == Core::GameSystem::GBC || system == Core::GameSystem::GBA)
        return &m_mgba;
    if (system == Core::GameSystem::NDS)
        return &m_melonDs;
    return nullptr;
}

const CoresWidget::CoreRow* CoresWidget::rowFor(Core::GameSystem system) const
{
    return const_cast<CoresWidget*>(this)->rowFor(system);
}

} // namespace Pocket::App
