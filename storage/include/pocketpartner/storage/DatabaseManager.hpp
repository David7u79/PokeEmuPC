#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace PocketPartner::Storage {

struct DatabaseConfig {
    std::string dbPath;
};

class DatabaseManager {
public:
    explicit DatabaseManager(DatabaseConfig config);
    ~DatabaseManager();

    // Disable copy
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool initialize();
    bool executeSchemaMigrations();
    bool close();

    bool isInitialized() const { return m_initialized; }

    // Execute arbitrary SQL statement (e.g. DDL / non-query DML)
    bool execute(const std::string& sql);

    // Query helper for scalar string key-value store
    bool setKV(const std::string& key, const std::string& value);
    std::optional<std::string> getKV(const std::string& key);

private:
    DatabaseConfig m_config;
    bool m_initialized{false};
    void* m_dbHandle{nullptr};
};

} // namespace PocketPartner::Storage
