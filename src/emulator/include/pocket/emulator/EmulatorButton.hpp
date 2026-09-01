#pragma once

#include <cstdint>
#include <string>

namespace Pocket::Emulator {

enum class EmulatorButton : uint8_t {
    A,
    B,
    X,
    Y,
    L,
    R,
    Select,
    Start,
    Up,
    Down,
    Left,
    Right
};

inline std::string buttonToString(EmulatorButton btn) {
    switch (btn) {
        case EmulatorButton::A:      return "A";
        case EmulatorButton::B:      return "B";
        case EmulatorButton::X:      return "X";
        case EmulatorButton::Y:      return "Y";
        case EmulatorButton::L:      return "L";
        case EmulatorButton::R:      return "R";
        case EmulatorButton::Select: return "Select";
        case EmulatorButton::Start:  return "Start";
        case EmulatorButton::Up:     return "Up";
        case EmulatorButton::Down:   return "Down";
        case EmulatorButton::Left:   return "Left";
        case EmulatorButton::Right:  return "Right";
    }
    return "Unknown";
}

} // namespace Pocket::Emulator
