#include "pocket/core/GameSystem.hpp"
#include <QFileInfo>
#include <QString>
#include <algorithm>

namespace Pocket::Core {

std::string GameSystemUtils::toString(GameSystem system) {
    switch (system) {
        case GameSystem::GB:  return "GB";
        case GameSystem::GBC: return "GBC";
        case GameSystem::GBA: return "GBA";
        case GameSystem::NDS: return "NDS";
        default:              return "Unknown";
    }
}

GameSystem GameSystemUtils::fromString(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "GB")  return GameSystem::GB;
    if (upper == "GBC") return GameSystem::GBC;
    if (upper == "GBA") return GameSystem::GBA;
    if (upper == "NDS") return GameSystem::NDS;
    return GameSystem::Unknown;
}

std::optional<GameSystem> GameSystemUtils::detectFromExtension(const std::string& filePathOrExt) {
    QFileInfo info(QString::fromStdString(filePathOrExt));
    QString ext = info.suffix().toLower();

    if (ext == "gb")  return GameSystem::GB;
    if (ext == "gbc") return GameSystem::GBC;
    if (ext == "gba") return GameSystem::GBA;
    if (ext == "nds") return GameSystem::NDS;

    return std::nullopt;
}

std::string GameSystemUtils::defaultExtension(GameSystem system) {
    switch (system) {
        case GameSystem::GB:  return ".gb";
        case GameSystem::GBC: return ".gbc";
        case GameSystem::GBA: return ".gba";
        case GameSystem::NDS: return ".nds";
        default:              return "";
    }
}

} // namespace Pocket::Core
