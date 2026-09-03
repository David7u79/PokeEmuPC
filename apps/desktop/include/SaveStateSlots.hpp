#pragma once

#include <QFileInfo>
#include <QString>

namespace Pocket::App {

constexpr int kSaveStateSlotCount = 5;

inline QString saveStatePath(const QString& savePath, int slot) {
    if (savePath.isEmpty() || slot < 0 || slot > kSaveStateSlotCount)
        return {};
    const QFileInfo info(savePath);
    const QString suffix = slot == 0 ? QStringLiteral("auto") : QString::number(slot);
    return info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + QStringLiteral(".state") + suffix;
}

} // namespace Pocket::App
