#pragma once

#include <QString>

namespace Pocket::CompanionApp {

class CompanionStartupManager {
public:
    static bool isAutostartEnabled();
    static bool setAutostartEnabled(bool enable);
};

} // namespace Pocket::CompanionApp
