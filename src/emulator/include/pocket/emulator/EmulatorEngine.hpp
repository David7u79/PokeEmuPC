#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "pocket/emulator/PersistentGameSave.hpp"
#include "pocket/emulator/SaveState.hpp"

namespace Pocket::Emulator {

enum class EmulatorButton {
    Up = 0,
    Down,
    Left,
    Right,
    A,
    B,
    L,
    R,
    Start,
    Select,
    X,
    Y
};

class EmulatorEngine {
public:
    virtual ~EmulatorEngine() = default;

    virtual bool loadRom(const std::string& romPath) = 0;
    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual bool isPaused() const = 0;

    virtual void sendButtonEvent(EmulatorButton button, bool pressed) = 0;

    virtual PersistentGameSave getPersistentSave() const = 0;
    virtual bool loadPersistentSave(const PersistentGameSave& save) = 0;

    using VideoFrameCallback = std::function<void(const uint8_t* pixels, int width, int height, size_t pitch)>;
    using AudioSampleCallback = std::function<void(const int16_t* samples, size_t frames)>;

    virtual void setVideoFrameCallback(VideoFrameCallback callback) = 0;
    virtual void setAudioSampleCallback(AudioSampleCallback callback) = 0;
};

} // namespace Pocket::Emulator
