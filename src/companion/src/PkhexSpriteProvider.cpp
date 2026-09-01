#include "pocket/companion/PkhexSpriteProvider.hpp"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <sstream>
#include <iomanip>

namespace Pocket::Companion {

PkhexSpriteProvider::PkhexSpriteProvider(const std::string& assetBaseDir) {
    if (!assetBaseDir.empty()) {
        m_assetBaseDir = assetBaseDir;
    } else {
        QString appDir = QCoreApplication::applicationDirPath();
        m_assetBaseDir = (appDir + "/assets/pokemon/pkhex").toStdString();
    }
}

SpriteResult PkhexSpriteProvider::resolve(const SpriteKey& key) {
    SpriteResult result;
    result.providerName = "PkhexSpriteProvider";

    if (!key.isValid()) return result;

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << key.speciesId;
    std::string idStr = ss.str();

    if (key.shiny) {
        QString shinyPath = QString("%1/shiny/%2.png")
            .arg(QString::fromStdString(m_assetBaseDir))
            .arg(QString::fromStdString(idStr));

        if (QFileInfo::exists(shinyPath)) {
            result.success = true;
            result.imagePath = shinyPath.toStdString();
            result.isShiny = true;
            result.isFallback = false;
            return result;
        }
    }

    QString regularPath = QString("%1/regular/%2.png")
        .arg(QString::fromStdString(m_assetBaseDir))
        .arg(QString::fromStdString(idStr));

    if (QFileInfo::exists(regularPath)) {
        result.success = true;
        result.imagePath = regularPath.toStdString();
        result.isShiny = false;
        result.isFallback = key.shiny;
        return result;
    }

    return result;
}

} // namespace Pocket::Companion
