#include "pocket/storage/GameMetadataResolver.hpp"
#include <fstream>
#include <algorithm>
#include <regex>
#include <QFileInfo>

namespace Pocket::Storage {

GameMetadataResolver::GameMetadataResolver() {
    // Populate Initial Embedded Libretro Database entries for Pokémon titles
    m_database.push_back({"2E567A20", {"Pokemon - Emerald Version", "Nintendo - Game Boy Advance", "USA", "2005", "Game Freak", "Nintendo", "RPG", "ExactHash"}});
    m_database.push_back({"40ACFBB3", {"Pokemon - FireRed Version", "Nintendo - Game Boy Advance", "USA", "2004", "Game Freak", "Nintendo", "RPG", "ExactHash"}});
    m_database.push_back({"3649E828", {"Pokemon - LeafGreen Version", "Nintendo - Game Boy Advance", "USA", "2004", "Game Freak", "Nintendo", "RPG", "ExactHash"}});
    m_database.push_back({"4C883F20", {"Pokemon - Ruby Version", "Nintendo - Game Boy Advance", "USA", "2003", "Game Freak", "Nintendo", "RPG", "ExactHash"}});
    m_database.push_back({"93BB7E67", {"Pokemon - Sapphire Version", "Nintendo - Game Boy Advance", "USA", "2003", "Game Freak", "Nintendo", "RPG", "ExactHash"}});
}

void GameMetadataResolver::addDatabaseEntry(const std::string& crc32, const GameMetadata& metadata) {
    m_database.push_back({crc32, metadata});
}

std::string GameMetadataResolver::normalizeFilename(const std::string& filename) {
    std::string clean = filename;
    // Remove extension
    size_t lastDot = clean.find_last_of('.');
    if (lastDot != std::string::npos) {
        clean = clean.substr(0, lastDot);
    }
    // Remove bracket tags e.g. (USA, Europe), [!], (v1.1)
    clean = std::regex_replace(clean, std::regex("\\(.*?\\)|\\[.*?\\]"), "");
    // Trim whitespace
    clean.erase(0, clean.find_first_not_of(" \t\n\r"));
    clean.erase(clean.find_last_not_of(" \t\n\r") + 1);

    return clean.empty() ? filename : clean;
}

GameMetadata GameMetadataResolver::resolveFromHeader(const std::vector<uint8_t>& header, const std::string& extension) {
    GameMetadata meta;
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if ((ext == ".gba") && header.size() >= 0xC0) {
        meta.platform = "Nintendo - Game Boy Advance";
        std::string rawTitle(reinterpret_cast<const char*>(header.data() + 0xA0), 12);
        rawTitle.erase(rawTitle.find_last_not_of(" \t\n\r\0") + 1);
        if (!rawTitle.empty()) {
            meta.canonicalTitle = rawTitle;
            meta.matchedBy = "HeaderMetadata";
            return meta;
        }
    } else if ((ext == ".nds") && header.size() >= 0x20) {
        meta.platform = "Nintendo - Nintendo DS";
        std::string rawTitle(reinterpret_cast<const char*>(header.data() + 0x00), 12);
        rawTitle.erase(rawTitle.find_last_not_of(" \t\n\r\0") + 1);
        if (!rawTitle.empty()) {
            meta.canonicalTitle = rawTitle;
            meta.matchedBy = "HeaderMetadata";
            return meta;
        }
    } else if ((ext == ".gb" || ext == ".gbc") && header.size() >= 0x0144) {
        meta.platform = (ext == ".gbc") ? "Nintendo - Game Boy Color" : "Nintendo - Game Boy";
        std::string rawTitle(reinterpret_cast<const char*>(header.data() + 0x0134), 16);
        rawTitle.erase(rawTitle.find_last_not_of(" \t\n\r\0") + 1);
        if (!rawTitle.empty()) {
            meta.canonicalTitle = rawTitle;
            meta.matchedBy = "HeaderMetadata";
            return meta;
        }
    }

    return meta;
}

GameMetadata GameMetadataResolver::resolve(const std::string& romFilePath, const Pocket::Core::RomFingerprint& fingerprint) {
    // 1. Exact Fingerprint Match
    if (!fingerprint.crc32.empty()) {
        for (const auto& entry : m_database) {
            if (entry.crc32 == fingerprint.crc32) {
                GameMetadata matched = entry.metadata;
                matched.matchedBy = "ExactHash";
                return matched;
            }
        }
    }

    // 2. ROM Header Metadata
    std::ifstream file(romFilePath, std::ios::binary);
    if (file.is_open()) {
        std::vector<uint8_t> header(512, 0x00);
        file.read(reinterpret_cast<char*>(header.data()), 512);
        file.close();

        QFileInfo info(QString::fromStdString(romFilePath));
        GameMetadata headerMeta = resolveFromHeader(header, info.suffix().toStdString());
        if (headerMeta.isValid()) {
            return headerMeta;
        }
    }

    // 3. Normalized Filename Fallback
    QFileInfo info(QString::fromStdString(romFilePath));
    std::string baseName = info.fileName().toStdString();
    std::string cleanName = normalizeFilename(baseName);

    GameMetadata fallback;
    fallback.canonicalTitle = cleanName;
    fallback.matchedBy = "FilenameFallback";

    std::string ext = info.suffix().toLower().toStdString();
    if (ext == "gba") fallback.platform = "Nintendo - Game Boy Advance";
    else if (ext == "nds") fallback.platform = "Nintendo - Nintendo DS";
    else if (ext == "gbc") fallback.platform = "Nintendo - Game Boy Color";
    else if (ext == "gb") fallback.platform = "Nintendo - Game Boy";

    return fallback;
}

} // namespace Pocket::Storage
