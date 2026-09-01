#include "PowerStatusMonitor.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Pocket::CompanionApp {

PowerInfo PowerStatusMonitor::queryPowerStatus() {
    PowerInfo info;

#ifdef _WIN32
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        info.isOnBattery = (status.ACLineStatus == 0);
        info.isBatterySaverActive = ((status.SystemStatusFlag & 0x01) != 0);
        if (status.BatteryLifePercent != 255) {
            info.batteryPercentage = static_cast<int>(status.BatteryLifePercent);
        }
    }
#endif

    return info;
}

} // namespace Pocket::CompanionApp
