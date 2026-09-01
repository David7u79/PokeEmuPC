#pragma once

#include "pocket/emulator/EmulatorEngine.hpp"
#include "pocket/save/SaveSessionCoordinator.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace Pocket::Emulator {

class MelonDsEngine : public EmulatorEngine {
public:
    explicit MelonDsEngine(std::shared_ptr<Pocket::Save::SaveSessionCoordinator> coordinator = std::make_shared<Pocket::Save::SaveSessionCoordinator>());
    ~MelonDsEngine() override;

    bool loadRom(const std::string& romPath) override;
    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    bool isRunning() const override { return m_isRunning; }
    bool isPaused() const override { return m_isPaused; }

    void sendButtonEvent(EmulatorButton button, bool pressed) override;

    PersistentGameSave getPersistentSave() const override;
    bool loadPersistentSave(const PersistentGameSave& save) override;

    void setVideoFrameCallback(VideoFrameCallback callback) override { m_videoCallback = callback; }
    void setAudioSampleCallback(AudioSampleCallback callback) override { m_audioCallback = callback; }

    std::string loadedRomPath() const { return m_romPath; }
    std::string saveFilePath() const { return m_savePath; }

    // Dual-Screen Framebuffers (256x192 RGBA 32-bit per screen)
    static constexpr int kScreenWidth = 256;
    static constexpr int kScreenHeight = 192;
    static constexpr size_t kScreenSizeBytes = kScreenWidth * kScreenHeight * 4;

    const uint8_t* topFramebuffer() const { return m_topFramebuffer.data(); }
    const uint8_t* bottomFramebuffer() const { return m_bottomFramebuffer.data(); }

    // Touchscreen Input (0..255, 0..191)
    void sendTouchInput(int x, int y, bool isPressed);
    void getTouchInput(int& x, int& y, bool& isPressed) const;

    // Optional Custom BIOS & Firmware Config
    void setBios7Path(const std::string& path) { m_bios7Path = path; }
    void setBios9Path(const std::string& path) { m_bios9Path = path; }
    void setFirmwarePath(const std::string& path) { m_firmwarePath = path; }

private:
    std::shared_ptr<Pocket::Save::SaveSessionCoordinator> m_saveCoordinator;

    bool m_isLoaded{false};
    bool m_isRunning{false};
    bool m_isPaused{false};

    std::string m_romPath;
    std::string m_savePath;

    std::string m_bios7Path;
    std::string m_bios9Path;
    std::string m_firmwarePath;

    std::vector<uint8_t> m_topFramebuffer;
    std::vector<uint8_t> m_bottomFramebuffer;

    int m_touchX{0};
    int m_touchY{0};
    bool m_touchPressed{false};

    VideoFrameCallback m_videoCallback;
    AudioSampleCallback m_audioCallback;
};

} // namespace Pocket::Emulator
