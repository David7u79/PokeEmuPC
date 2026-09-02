#include "CoresWidget.hpp"
#include "pocket/emulator/LibretroCoreProbe.hpp"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
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
    auto addRow = [this, layout](CoreRow& row) {
        auto* name = new QLabel(QString("<b>%1</b> — expected core: %2").arg(row.displayName, row.expectedCore), this);
        layout->addWidget(name);

        row.pathEdit = new QLineEdit(this);
        row.pathEdit->setReadOnly(true);
        row.pathEdit->setClearButtonEnabled(false);
        row.pathEdit->setToolTip(row.pathEdit->text());
        auto* browseButton = new QPushButton("Browse...", this);
        auto* importButton = new QPushButton("Import...", this);
        auto* clearButton = new QPushButton("Clear", this);
        auto* pathLayout = new QHBoxLayout();
        pathLayout->addWidget(row.pathEdit);
        pathLayout->addWidget(browseButton);
        pathLayout->addWidget(importButton);
        pathLayout->addWidget(clearButton);
        layout->addLayout(pathLayout);

        row.statusLabel = new QLabel(this);
        row.statusLabel->setWordWrap(true);
        layout->addWidget(row.statusLabel);
        connect(browseButton, &QPushButton::clicked, this, [this, &row] { browse(row); });
        connect(importButton, &QPushButton::clicked, this, [this, &row] { importCore(row); });
        connect(clearButton, &QPushButton::clicked, this, [this, &row] { clear(row); });
    };

    addRow(m_mgba);
    addRow(m_melonDs);
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
        row.statusLabel->setText("Not configured");
        row.statusLabel->setStyleSheet("color: gray;");
        return;
    }
    if (!QFileInfo::exists(path)) {
        row.statusLabel->setText("File not found");
        row.statusLabel->setStyleSheet("color: red;");
        return;
    }

    const auto description = Emulator::probeLibretroCore(path.toStdString());
    if (!description.valid) {
        row.statusLabel->setText(QString::fromStdString(description.error));
        row.statusLabel->setStyleSheet("color: red;");
    } else if (!Emulator::coreSupportsSystem(description, row.system)) {
        row.statusLabel->setText("This core does not report support for " + row.displayName);
        row.statusLabel->setStyleSheet("color: #b58900;");
    } else {
        row.statusLabel->setText(QString::fromStdString(description.libraryName + " " + description.libraryVersion).trimmed());
        row.statusLabel->setStyleSheet("color: green;");
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
