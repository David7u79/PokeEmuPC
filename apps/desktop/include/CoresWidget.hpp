#pragma once

#include "pocket/core/GameSystem.hpp"
#include <QWidget>

class QLineEdit;
class QLabel;

namespace Pocket::App {

class CoresWidget : public QWidget {
    Q_OBJECT
public:
    explicit CoresWidget(QWidget* parent = nullptr);
    void refresh();
    QString corePath(Core::GameSystem system) const;

signals:
    void corePathChanged(Core::GameSystem system, const QString& path);

private:
    struct CoreRow {
        Core::GameSystem system;
        QString displayName;
        QString expectedCore;
        QString settingsKey;
        QLineEdit* pathEdit{nullptr};
        QLabel* statusLabel{nullptr};
    };

    void browse(CoreRow& row);
    void importCore(CoreRow& row);
    void clear(CoreRow& row);
    bool validateAndSave(CoreRow& row, const QString& path);
    void updateStatus(CoreRow& row);
    CoreRow* rowFor(Core::GameSystem system);
    const CoreRow* rowFor(Core::GameSystem system) const;

    CoreRow m_mgba{Core::GameSystem::GBA, "Game Boy / Color / Advance", "mGBA", "emulator/mgbaCorePath"};
    CoreRow m_melonDs{Core::GameSystem::NDS, "Nintendo DS", "melonDS DS", "emulator/melonDsCorePath"};
};

} // namespace Pocket::App
