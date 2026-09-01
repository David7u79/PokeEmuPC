#pragma once

#include <string>

namespace Pocket::Companion {

enum class CompanionCommandType {
    Feed,
    Pet,
    Play,
    Rest
};

struct FeedCompanionCommand {
    std::string companionId;
    double foodAmount{30.0};
};

struct PetCompanionCommand {
    std::string companionId;
};

struct PlayWithCompanionCommand {
    std::string companionId;
    int durationMinutes{15};
};

struct RestCompanionCommand {
    std::string companionId;
    int durationMinutes{60};
};

} // namespace Pocket::Companion
