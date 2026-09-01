#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QString>
#include <string>
#include <memory>
#include "pocket/save/SaveSessionState.hpp"
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/save/SaveSessionCoordinator.hpp"

namespace Pocket::Save {

class ExternalSaveWatcher : public QObject {
    Q_OBJECT
public:
    explicit ExternalSaveWatcher(
        std::shared_ptr<SaveSessionCoordinator> coordinator = std::make_shared<SaveSessionCoordinator>(),
        QObject *parent = nullptr
    );
    ~ExternalSaveWatcher() override = default;

    bool setMonitoredSavePath(const std::string& savePath);
    std::string monitoredSavePath() const { return m_monitoredSavePath; }

    void setSyncMode(ExternalSyncMode mode) { m_syncMode = mode; }
    ExternalSyncMode syncMode() const { return m_syncMode; }

    std::string lastSaveHash() const { return m_lastSaveHash; }

    void forceCheck();

signals:
    void saveUpdated(const QString& savePath, const Pocket::Save::SaveParseResult& result, const QString& newHash);
    void saveFileRemoved(const QString& savePath);

private slots:
    void onFileChanged(const QString& path);
    void onDebounceTimeout();

private:
    void processFileChange();

    std::shared_ptr<SaveSessionCoordinator> m_coordinator;
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;

    std::string m_monitoredSavePath;
    std::string m_lastSaveHash;
    ExternalSyncMode m_syncMode{ExternalSyncMode::ReadOnlyExternal};

    Gen3SaveParser m_parser;
};

} // namespace Pocket::Save
