#pragma once

#include <QWidget>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "ControllerMapperWidget.hpp"
#include "CoresWidget.hpp"

namespace Pocket::App {

class DiagnosticsWidget;

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager,
                   std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent = nullptr);

    ControllerMapperWidget* controllerMapper() const { return m_controllerMapper; }
    CoresWidget* coresWidget() const { return m_coresWidget; }
    DiagnosticsWidget* diagnosticsWidget() const { return m_diagnosticsWidget; }

signals:
    void coreLibraryPathChanged(const QString& path);
    void melonDsCorePathChanged(const QString& path);

private:
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_db;
    ControllerMapperWidget* m_controllerMapper{nullptr};
    CoresWidget* m_coresWidget{nullptr};
    DiagnosticsWidget* m_diagnosticsWidget{nullptr};
};

} // namespace Pocket::App
