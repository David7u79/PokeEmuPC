#pragma once

#include <QWidget>
#include <QLabel>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"

namespace Pocket::App {

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager, QWidget *parent = nullptr);

private:
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_db;
    QLabel *m_dbPathLabel{nullptr};
    QLabel *m_versionLabel{nullptr};
};

} // namespace Pocket::App
