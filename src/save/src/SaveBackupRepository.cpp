#include "pocket/save/SaveBackupRepository.hpp"
#include <QUuid>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace Pocket::Save {

SaveBackupRepository::SaveBackupRepository(const std::string& backupDirectory)
    : m_backupDir(backupDirectory) {
    if (m_backupDir.empty()) {
        m_backupDir = (std::filesystem::temp_directory_path() / "pocket_backups").string();
    }
    std::filesystem::create_directories(m_backupDir);
}

SaveBackup SaveBackupRepository::createBackup(const std::string& gameId, const std::string& savePath, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    SaveBackup backup;
    backup.backupId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    backup.gameId = gameId;
    backup.timestamp = QDateTime::currentSecsSinceEpoch();
    backup.reason = reason;

    // Read source save content & calculate SHA-256
    std::ifstream file(savePath, std::ios::binary);
    if (!file.is_open()) return backup;

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (buffer.empty()) return backup; // 12-Step Save Safety: Do not back up empty data

    QByteArray hashData = QCryptographicHash::hash(QByteArray(buffer.data(), static_cast<qsizetype>(buffer.size())), QCryptographicHash::Sha256);
    backup.sourceHash = hashData.toHex().toStdString();

    // Destination backup file path
    std::string backupFileName = gameId + "_" + std::to_string(backup.timestamp) + "_" + backup.backupId.substr(0, 8) + ".sav";
    std::filesystem::path dstPath = std::filesystem::path(m_backupDir) / backupFileName;
    backup.path = dstPath.string();

    // Write backup file
    std::ofstream dstFile(dstPath, std::ios::binary);
    if (dstFile.is_open()) {
        dstFile.write(buffer.data(), buffer.size());
        dstFile.close();

        m_memoryBackups[gameId].push_back(backup);
        enforceRetentionPolicy(gameId, 10); // Default retention limit: 10 max backups
    }

    return backup;
}

std::vector<SaveBackup> SaveBackupRepository::getBackupsForGame(const std::string& gameId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_memoryBackups.find(gameId);
    if (it != m_memoryBackups.end()) {
        return it->second;
    }
    return {};
}

bool SaveBackupRepository::restoreBackup(const SaveBackup& backup, const std::string& targetSavePath) {
    if (backup.path.empty() || !std::filesystem::exists(backup.path)) return false;

    std::ifstream src(backup.path, std::ios::binary);
    if (!src.is_open()) return false;

    std::ofstream dst(targetSavePath, std::ios::binary);
    if (!dst.is_open()) return false;

    dst << src.rdbuf();
    return dst.good();
}

void SaveBackupRepository::enforceRetentionPolicy(const std::string& gameId, size_t maxBackups) {
    auto& list = m_memoryBackups[gameId];
    if (list.size() <= maxBackups) return;

    // Sort by timestamp ascending (oldest first)
    std::sort(list.begin(), list.end(), [](const SaveBackup& a, const SaveBackup& b) {
        return a.timestamp < b.timestamp;
    });

    while (list.size() > maxBackups) {
        SaveBackup oldest = list.front();
        list.erase(list.begin());

        if (!oldest.path.empty() && std::filesystem::exists(oldest.path)) {
            std::filesystem::remove(oldest.path);
        }
    }
}

} // namespace Pocket::Save
