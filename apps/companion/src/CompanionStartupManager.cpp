#include "CompanionStartupManager.hpp"
#include <QSettings>
#include <QCoreApplication>

namespace Pocket::CompanionApp {

static const char* kRegKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const char* kValueName = "PocketPartnerCompanion";

bool CompanionStartupManager::isAutostartEnabled() {
#ifdef _WIN32
    QSettings bootSettings(kRegKey, QSettings::NativeFormat);
    return bootSettings.contains(kValueName);
#else
    return false;
#endif
}

bool CompanionStartupManager::setAutostartEnabled(bool enable) {
#ifdef _WIN32
    QSettings bootSettings(kRegKey, QSettings::NativeFormat);
    if (enable) {
        QString appPath = QString("\"%1\"").arg(QCoreApplication::applicationFilePath().replace('/', '\\'));
        bootSettings.setValue(kValueName, appPath);
    } else {
        bootSettings.remove(kValueName);
    }
    return true;
#else
    return false;
#endif
}

} // namespace Pocket::CompanionApp
