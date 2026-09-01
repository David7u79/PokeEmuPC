#pragma once

#include "pocket/companion/CompanionLink.hpp"
#include "pocket/save/CreatureSaveParser.hpp"

namespace Pocket::Save {

class CompanionReidentifier {
public:
    static Pocket::Companion::CompanionLink reidentify(
        const Pocket::Companion::CompanionLink& currentLink,
        const SaveParseResult& parsedSave,
        const std::string& currentSaveHash
    );

    static Pocket::Companion::CompanionLink createLinkFromCreature(
        const Creature& creature,
        int gameId,
        const std::string& saveHash
    );
};

} // namespace Pocket::Save
