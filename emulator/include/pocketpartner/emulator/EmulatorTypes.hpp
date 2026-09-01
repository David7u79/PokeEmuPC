#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <variant>

namespace PocketPartner::Emulator {

enum class TargetSystem {
    GameBoy,
    GameBoyColor,
    GameBoyAdvance,
    NintendoDS
};

enum class EmulatorButton {
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

enum class EmulatorState {
    Unloaded,
    Loaded,
    Running,
    Paused,
    Stopped,
    Error
};

struct GameLaunchRequest {
    std::string romPath;
    std::string savePath;
    TargetSystem system;
    bool readOnlySave{false};
};

template <typename T>
struct Result {
    bool isSuccess{true};
    T value{};
    std::string errorMessage;

    static Result<T> ok(T val = {}) {
        return Result<T>{true, std::move(val), ""};
    }

    static Result<T> fail(std::string msg) {
        return Result<T>{false, {}, std::move(msg)};
    }
};

using VoidResult = Result<void*>;

} // namespace PocketPartner::Emulator
