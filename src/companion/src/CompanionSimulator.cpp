#include "pocket/companion/CompanionSimulator.hpp"

namespace Pocket::Companion {

CompanionSimulator::CompanionSimulator(std::shared_ptr<IClock> clock)
    : m_clock(std::move(clock)) {}

CompanionState CompanionSimulator::calculateCurrentState(const CompanionState& initialState) const {
    int64_t now = m_clock->nowSecs();
    if (initialState.lastCalculatedAt <= 0 || now <= initialState.lastCalculatedAt) {
        CompanionState state = initialState;
        if (state.lastCalculatedAt <= 0) {
            state.lastCalculatedAt = now;
        }
        return state;
    }

    int64_t elapsedSecs = now - initialState.lastCalculatedAt;
    double hours = static_cast<double>(elapsedSecs) / 3600.0;

    CompanionState state = initialState;

    // Decay rates per hour
    state.hunger = clampValue(state.hunger - (4.0 * hours));
    state.energy = clampValue(state.energy - (2.5 * hours));
    state.mood   = clampValue(state.mood   - (3.0 * hours));
    state.fatigue= clampValue(state.fatigue+ (2.0 * hours));

    state.lastCalculatedAt = now;
    return state;
}

CompanionState CompanionSimulator::executeFeed(const CompanionState& initialState, const FeedCompanionCommand& cmd) const {
    CompanionState state = calculateCurrentState(initialState);
    int64_t now = m_clock->nowSecs();

    state.hunger = clampValue(state.hunger + cmd.foodAmount);
    state.mood   = clampValue(state.mood + 5.0);
    state.bond.addXp(10);

    state.lastFedAt = now;
    state.lastInteractionAt = now;
    state.lastCalculatedAt = now;
    return state;
}

CompanionState CompanionSimulator::executePet(const CompanionState& initialState, const PetCompanionCommand&) const {
    CompanionState state = calculateCurrentState(initialState);
    int64_t now = m_clock->nowSecs();

    state.mood   = clampValue(state.mood + 15.0);
    state.energy = clampValue(state.energy + 5.0);
    state.bond.addXp(15);

    state.lastInteractionAt = now;
    state.lastCalculatedAt = now;
    return state;
}

CompanionState CompanionSimulator::executePlay(const CompanionState& initialState, const PlayWithCompanionCommand&) const {
    CompanionState state = calculateCurrentState(initialState);
    int64_t now = m_clock->nowSecs();

    state.mood    = clampValue(state.mood + 25.0);
    state.energy  = clampValue(state.energy - 15.0);
    state.fatigue = clampValue(state.fatigue + 10.0);
    state.bond.addXp(20);

    state.lastPlayedAt = now;
    state.lastInteractionAt = now;
    state.lastCalculatedAt = now;
    return state;
}

CompanionState CompanionSimulator::executeRest(const CompanionState& initialState, const RestCompanionCommand&) const {
    CompanionState state = calculateCurrentState(initialState);
    int64_t now = m_clock->nowSecs();

    state.energy  = 100.0;
    state.fatigue = 0.0;
    state.bond.addXp(5);

    state.lastRestedAt = now;
    state.lastInteractionAt = now;
    state.lastCalculatedAt = now;
    return state;
}

} // namespace Pocket::Companion
