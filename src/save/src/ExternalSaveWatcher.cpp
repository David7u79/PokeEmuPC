#include "pocket/save/ExternalSaveWatcher.hpp"
#include "pocket/save/FileStabilityVerifier.hpp"
#include "pocket/save/Gen3SaveEditor.hpp"
#include <fstream>
#include <QFileInfo>

namespace Pocket::Save {

ExternalSaveWatcher::ExternalSaveWatcher(
    std::shared_ptr<SaveSessionCoordinator> coordinator,
    QObject *parent
) : QObject(parent), m_coordinator(std::move(coordinator)) {

    m_debounceTimer.setSingleShot(true);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ExternalSaveWatcher::onFileChanged);
    connect(&m_debounceTimer, &QTimer::timeout, this, &ExternalSaveWatcher::onDebounceTimeout);
}

bool ExternalSaveWatcher::setMonitoredSavePath(const std::string& savePath) {
    if (!m_monitoredSavePath.empty()) {
        m_watcher.removePath(QString::fromStdString(m_monitoredSavePath));
        m_coordinator->setExternalProcessActive(m_monitoredSavePath, false);
        m_monitoredSavePath.clear();
        m_lastSaveHash.clear();
    }

    if (savePath.empty()) return true;

    QFileInfo info(QString::fromStdString(savePath));
    if (!info.exists()) return false;

    m_monitoredSavePath = savePath;
    m_watcher.addPath(QString::fromStdString(m_monitoredSavePath));

    if (m_syncMode == ExternalSyncMode::ReadOnlyExternal) {
        m_coordinator->setExternalProcessActive(m_monitoredSavePath, true);
    }

    forceCheck();
    return true;
}

void ExternalSaveWatcher::onFileChanged(const QString& path) {
    Q_UNUSED(path);
    // Restart 100ms single-shot debounce timer
    m_debounceTimer.start(100);
}

void ExternalSaveWatcher::onDebounceTimeout() {
    processFileChange();
}

void ExternalSaveWatcher::forceCheck() {
    processFileChange();
}

void ExternalSaveWatcher::processFileChange() {
    if (m_monitoredSavePath.empty()) return;

    QString qPath = QString::fromStdString(m_monitoredSavePath);
    QFileInfo info(qPath);

    if (!info.exists()) {
        emit saveFileRemoved(qPath);
        return;
    }

    // Ensure filesystem size & timestamp are stable
    if (!FileStabilityVerifier::isFileStable(m_monitoredSavePath)) {
        return;
    }

    // Read buffer and compute hash
    std::ifstream file(m_monitoredSavePath, std::ios::binary);
    if (!file.is_open()) return;

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (buffer.size() < 131072) {
        // Invalid or truncated partial save file, ignore until fully written
        return;
    }

    std::string newHash = Gen3SaveEditor::calculateSha256(buffer);
    if (newHash == m_lastSaveHash) {
        return; // No content change
    }

    SaveParseResult result = m_parser.parseSaveBuffer(buffer);
    if (result.status != SaveParseStatus::Success) {
        return; // Invalid save checksum, ignore partial write
    }

    m_lastSaveHash = newHash;

    // Re-add to watcher if atomic replace removed the file link
    if (!m_watcher.files().contains(qPath)) {
        m_watcher.addPath(qPath);
    }

    emit saveUpdated(qPath, result, QString::fromStdString(newHash));
}

} // namespace Pocket::Save
