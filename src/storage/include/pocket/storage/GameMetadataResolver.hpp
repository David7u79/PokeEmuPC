#pragma once

#include "pocket/core/RomFingerprint.hpp"
#include <string>
#include <vector>

namespace Pocket::Storage {

struct GameMetadata {
    std::string canonicalTitle;
    std::string platform;
    std::string region;
    std::string releaseYear;
    std::string developer;
    std::string publisher;
    std::string genre;
    std::string matchedBy; // "ExactHash", "HeaderMetadata", "FilenameFallback", "Unknown"

    bool isValid() const { return !canonicalTitle.empty(); }
};

class GameMetadataResolver {
public:
    GameMetadataResolver();

    GameMetadata resolve(const std::string& romFilePath, const Pocket::Core::RomFingerprint& fingerprint);
    GameMetadata resolveFromHeader(const std::vector<uint8_t>& headerData, const std::string& extension);
    static std::string normalizeFilename(const std::string& filename);

    void addDatabaseEntry(const std::string& crc32, const GameMetadata& metadata);

private:
    struct DbEntry {
        std::string crc32;
        GameMetadata metadata;
    };
    std::vector<DbEntry> m_database;
};

} // namespace Pocket::Storage
