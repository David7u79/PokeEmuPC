#include "pocket/companion/PlaceholderSpriteProvider.hpp"
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QStandardPaths>

namespace Pocket::Companion {

std::string PlaceholderSpriteProvider::ensurePlaceholderImageExists() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/pocket_assets";
    QDir().mkpath(cacheDir);
    QString path = cacheDir + "/placeholder.png";

    if (!QFile::exists(path)) {
        QImage img(64, 64, QImage::Format_ARGB32);
        img.fill(Qt::transparent);

        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing, false);

        // Draw a crisp Pokéball silhouette placeholder
        painter.setBrush(QColor(180, 70, 70));
        painter.setPen(Qt::NoPen);
        painter.drawPie(8, 8, 48, 48, 0, 180 * 16);

        painter.setBrush(QColor(220, 220, 220));
        painter.drawPie(8, 8, 48, 48, 180 * 16, 180 * 16);

        painter.setBrush(QColor(40, 40, 40));
        painter.drawRect(8, 30, 48, 4);

        painter.setBrush(QColor(255, 255, 255));
        painter.setPen(QPen(QColor(40, 40, 40), 2));
        painter.drawEllipse(26, 26, 12, 12);

        painter.end();
        img.save(path, "PNG");
    }

    return path.toStdString();
}

SpriteResult PlaceholderSpriteProvider::resolve(const SpriteKey& key) {
    Q_UNUSED(key);
    SpriteResult result;
    result.success = true;
    result.imagePath = ensurePlaceholderImageExists();
    result.providerName = "PlaceholderSpriteProvider";
    result.isShiny = false;
    result.isFallback = true;
    return result;
}

} // namespace Pocket::Companion
