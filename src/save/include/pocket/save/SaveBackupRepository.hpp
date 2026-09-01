#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <mutex>
#include "pocketpartner/storage/DatabaseManager.hpp"

namespace Pocket::Save {

struct SaveBackup {
    std::string backupId;
    std::string gameId;
    int64_t timestamp{0};
    std::string sourceHash;
    std::string reason;
    std::string path;
};

class SaveBackupRepository {
public:
    explicit SaveBackupRepository(const std::string& backupDirectory = "");

    SaveBackup createBackup(const std::string& gameId, const std::string& savePath, const std::string& reason);
    std::vector<SaveBackup> getBackupsForGame(const std::string& gameId) const;
    bool restoreBackup(const SaveBackup& backup, const std::string& targetSavePath);

    void enforceRetentionPolicy(const std::string& gameId, size_t maxBackups = 10);

private:
    std::string m_backupDir;
    mutable std::mutex m_mutex;
    std::map<std::string, std::vector<SaveBackup>> m_memoryBackups;
};

} // namespace Pocket::Save
