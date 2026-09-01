#pragma once

#include <memory>
#include <algorithm>
#include "pocket/companion/IClock.hpp"
#include "pocket/companion/CompanionState.hpp"
#include "pocket/companion/CompanionCommands.hpp"

namespace Pocket::Companion {

class CompanionSimulator {
public:
    explicit CompanionSimulator(std::shared_ptr<IClock> clock = std::make_shared<SystemClock>());

    CompanionState calculateCurrentState(const CompanionState& initialState) const;

    CompanionState executeFeed(const CompanionState& state, const FeedCompanionCommand& cmd) const;
    CompanionState executePet(const CompanionState& state, const PetCompanionCommand& cmd) const;
    CompanionState executePlay(const CompanionState& state, const PlayWithCompanionCommand& cmd) const;
    CompanionState executeRest(const CompanionState& state, const RestCompanionCommand& cmd) const;

    static double clampValue(double val) {
        return std::clamp(val, 0.0, 100.0);
    }

private:
    std::shared_ptr<IClock> m_clock;
};

} // namespace Pocket::Companion
