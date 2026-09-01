#include "pocket/save/CompanionReidentifier.hpp"
#include <chrono>

namespace Pocket::Save {

Pocket::Companion::CompanionLink CompanionReidentifier::createLinkFromCreature(
    const Creature& creature,
    int gameId,
    const std::string& saveHash
) {
    Pocket::Companion::CompanionLink link;
    link.gameId = gameId;
    link.generation = static_cast<int>(creature.generation);
    link.lastVerifiedSaveHash = saveHash;
    link.status = Pocket::Companion::LinkStatus::Linked;

    auto now = std::chrono::system_clock::now();
    link.lastUpdated = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Fingerprint
    link.fingerprint.personalityValue = creature.personalityValue;
    link.fingerprint.trainerId = creature.trainer.trainerId;
    link.fingerprint.secretId = creature.trainer.secretId;
    link.fingerprint.isFemale = creature.trainer.isFemale;
    link.fingerprint.otName = creature.trainer.trainerName;

    // Cached Canonical Display Attributes
    link.nickname = creature.nickname;
    link.speciesName = creature.speciesName;
    link.level = creature.level;
    link.gameFriendship = creature.friendship.rawValue();

    // Locator parsing
    if (creature.location.rfind("Party Slot ", 0) == 0) {
        link.locator.type = Pocket::Companion::LocationType::Party;
        link.locator.partySlot = std::stoi(creature.location.substr(11));
    } else {
        link.locator.type = Pocket::Companion::LocationType::Unknown;
    }

    return link;
}

Pocket::Companion::CompanionLink CompanionReidentifier::reidentify(
    const Pocket::Companion::CompanionLink& currentLink,
    const SaveParseResult& parsedSave,
    const std::string& currentSaveHash
) {
    Pocket::Companion::CompanionLink updatedLink = currentLink;
    updatedLink.lastVerifiedSaveHash = currentSaveHash;

    auto now = std::chrono::system_clock::now();
    updatedLink.lastUpdated = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    if (parsedSave.status != SaveParseStatus::Success) {
        updatedLink.status = Pocket::Companion::LinkStatus::NotFound;
        return updatedLink;
    }

    struct MatchCandidate {
        Creature creature;
        Pocket::Companion::CreatureLocator locator;
    };

    std::vector<MatchCandidate> matches;

    // 1. Check Party Slots
    for (size_t i = 0; i < parsedSave.party.size(); ++i) {
        const auto& pkmn = parsedSave.party[i];
        if (currentLink.fingerprint.matches(pkmn.personalityValue, pkmn.trainer.trainerId, pkmn.trainer.secretId)) {
            Pocket::Companion::CreatureLocator loc;
            loc.type = Pocket::Companion::LocationType::Party;
            loc.partySlot = static_cast<int>(i + 1);
            matches.push_back({pkmn, loc});
        }
    }

    // 2. Check PC Boxes
    for (size_t b = 0; b < parsedSave.boxes.size(); ++b) {
        for (size_t s = 0; s < parsedSave.boxes[b].size(); ++s) {
            const auto& pkmn = parsedSave.boxes[b][s];
            if (currentLink.fingerprint.matches(pkmn.personalityValue, pkmn.trainer.trainerId, pkmn.trainer.secretId)) {
                Pocket::Companion::CreatureLocator loc;
                loc.type = Pocket::Companion::LocationType::Box;
                loc.boxNumber = static_cast<int>(b + 1);
                loc.boxSlot = static_cast<int>(s + 1);
                matches.push_back({pkmn, loc});
            }
        }
    }

    // 3. Evaluate Match Results
    if (matches.size() == 1) {
        // Exactly one high-confidence match!
        const auto& found = matches[0];
        updatedLink.locator = found.locator;
        updatedLink.status = Pocket::Companion::LinkStatus::Linked;

        // Update cached canonical attributes (handles evolution, level up, nickname change!)
        updatedLink.nickname = found.creature.nickname;
        updatedLink.speciesName = found.creature.speciesName;
        updatedLink.level = found.creature.level;
        updatedLink.gameFriendship = found.creature.friendship.rawValue();

    } else if (matches.size() > 1) {
        // Ambiguous match (e.g. cloned creature)
        updatedLink.status = Pocket::Companion::LinkStatus::AmbiguousMatch;
    } else {
        // Not found in party or boxes
        updatedLink.status = Pocket::Companion::LinkStatus::NotFound;
    }

    return updatedLink;
}

} // namespace Pocket::Save
