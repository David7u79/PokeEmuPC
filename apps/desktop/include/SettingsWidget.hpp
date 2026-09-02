#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "ControllerMapperWidget.hpp"

namespace Pocket::App {

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                   std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent = nullptr);

    ControllerMapperWidget* controllerMapper() const { return m_controllerMapper; }

signals:
    void coreLibraryPathChanged(const QString& path);
    void melonDsCorePathChanged(const QString& path);

private slots:
    void browseCoreLibrary();
    void browseMelonDsCoreLibrary();

private:
    void updateCoreStatus(const QString& path);
    void updateMelonDsCoreStatus(const QString& path);
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_db;
    QLabel* m_dbPathLabel{nullptr};
    QLabel* m_versionLabel{nullptr};
    QLineEdit* m_corePathEdit{nullptr};
    QLabel* m_coreStatusLabel{nullptr};
    QLineEdit* m_melonDsCorePathEdit{nullptr};
    QLabel* m_melonDsCoreStatusLabel{nullptr};
    ControllerMapperWidget* m_controllerMapper{nullptr};
};

} // namespace Pocket::App
