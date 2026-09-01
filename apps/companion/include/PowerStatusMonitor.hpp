#pragma once

#include <QObject>

namespace Pocket::CompanionApp {

struct PowerInfo {
    bool isOnBattery{false};
    bool isBatterySaverActive{false};
    int batteryPercentage{-1}; // 0..100 or -1 if AC/unknown
};

class PowerStatusMonitor : public QObject {
    Q_OBJECT
public:
    explicit PowerStatusMonitor(QObject *parent = nullptr) : QObject(parent) {}
    ~PowerStatusMonitor() override = default;

    static PowerInfo queryPowerStatus();
};

} // namespace Pocket::CompanionApp
