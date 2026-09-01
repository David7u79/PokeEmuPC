#pragma once

#include "pocket/core/Game.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <map>

namespace Pocket::Storage {

enum class ImportResultStatus {
    Success,
    FileNotFound,
    InvalidSystem,
    DuplicatePath,
    DuplicateHash,
    DatabaseError
};

struct ImportResult {
    ImportResultStatus status{ImportResultStatus::DatabaseError};
    std::string errorMessage;
    std::optional<Core::Game> game;
};

class GameRepository {
public:
    explicit GameRepository(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager);

    ImportResult importGame(const std::string& romFilePath);
    
    std::vector<Core::Game> getAllGames() const;
    std::optional<Core::Game> getGameById(const Core::GameId& id) const;
    std::optional<Core::Game> getGameBySha256(const std::string& sha256) const;

    bool deleteGame(const Core::GameId& id);

    static std::string calculateSha256(const std::string& filePath);

private:
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> m_db;
    // In-memory cache for SHA-256 fingerprints to avoid hashing large files repeatedly
    mutable std::map<std::string, std::string> m_hashCache;
};

} // namespace Pocket::Storage
