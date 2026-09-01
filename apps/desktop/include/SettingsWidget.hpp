#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <memory>
#include "pocketpartner/storage/DatabaseManager.hpp"

namespace Pocket::App {

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager, QWidget *parent = nullptr);

signals:
    void coreLibraryPathChanged(const QString& path);

private slots:
    void browseCoreLibrary();

private:
    void updateCoreStatus(const QString& path);
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_db;
    QLabel *m_dbPathLabel{nullptr};
    QLabel *m_versionLabel{nullptr};
    QLineEdit *m_corePathEdit{nullptr};
    QLabel *m_coreStatusLabel{nullptr};
};

} // namespace Pocket::App
