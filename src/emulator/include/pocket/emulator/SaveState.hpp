#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Pocket::Emulator {

// SaveState represents full emulator CPU/RAM state snapshots (.ss1)
// Explicitly decoupled from PersistentGameSave.
class SaveState {
public:
    SaveState() = default;

    int slotIndex{0};
    std::string statePath;
    std::vector<uint8_t> stateData;

    bool isValid() const { return !stateData.empty(); }
};

} // namespace Pocket::Emulator
