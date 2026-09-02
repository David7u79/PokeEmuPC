#pragma once

#include "pocket/core/GameSystem.hpp"
#include <QSettings>
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

    // Where core paths are stored. Tests point this at a throwaway file; without
    // the seam they can only redirect QSettings process-wide, which silently
    // failed on Windows and let a test wipe the real configuration.
    static void setSettingsScope(const QString& organization, const QString& application);
    static QSettings openSettings();

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
