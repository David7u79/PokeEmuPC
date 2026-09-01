#include "pocket/companion/PokeSpriteProvider.hpp"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

namespace Pocket::Companion {

PokeSpriteProvider::PokeSpriteProvider(const std::string& assetBaseDir) {
    if (!assetBaseDir.empty()) {
        m_assetBaseDir = assetBaseDir;
    } else {
        QString appDir = QCoreApplication::applicationDirPath();
        m_assetBaseDir = (appDir + "/assets/pokemon/pokesprite").toStdString();
    }
}

std::string PokeSpriteProvider::speciesIdToSlug(uint16_t speciesId) {
    switch (speciesId) {
        case 1:   return "bulbasaur";
        case 2:   return "ivysaur";
        case 3:   return "venusaur";
        case 4:   return "charmander";
        case 5:   return "charmeleon";
        case 6:   return "charizard";
        case 7:   return "squirtle";
        case 8:   return "wartortle";
        case 9:   return "blastoise";
        case 25:  return "pikachu";
        case 133: return "eevee";
        case 150: return "mewtwo";
        case 151: return "mew";
        case 152: return "chikorita";
        case 155: return "cyndaquil";
        case 158: return "totodile";
        case 197: return "umbreon";
        case 252: return "treecko";
        case 253: return "grovyle";
        case 254: return "sceptile";
        case 255: return "torchic";
        case 258: return "mudkip";
        case 384: return "rayquaza";
        case 387: return "turtwig";
        case 388: return "grotle";
        case 389: return "torterra";
        case 494: return "victini";
        case 495: return "snivy";
        default:  return "species_" + std::to_string(speciesId);
    }
}

SpriteResult PokeSpriteProvider::resolve(const SpriteKey& key) {
    SpriteResult result;
    result.providerName = "PokeSpriteProvider";

    if (!key.isValid()) return result;

    std::string slug = speciesIdToSlug(key.speciesId);

    // If shiny requested, check shiny directory first
    if (key.shiny) {
        QString shinyPath = QString("%1/shiny/%2.png")
            .arg(QString::fromStdString(m_assetBaseDir))
            .arg(QString::fromStdString(slug));

        if (QFileInfo::exists(shinyPath)) {
            result.success = true;
            result.imagePath = shinyPath.toStdString();
            result.isShiny = true;
            result.isFallback = false;
            return result;
        }
    }

    // Check regular directory
    QString regularPath = QString("%1/regular/%2.png")
        .arg(QString::fromStdString(m_assetBaseDir))
        .arg(QString::fromStdString(slug));

    if (QFileInfo::exists(regularPath)) {
        result.success = true;
        result.imagePath = regularPath.toStdString();
        result.isShiny = false;
        result.isFallback = key.shiny; // Fallback if shiny was requested but only regular exists
        return result;
    }

    return result;
}

} // namespace Pocket::Companion
