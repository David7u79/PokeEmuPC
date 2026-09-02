#pragma once

#include "pocket/emulator/LibretroEngineBase.hpp"
#include "pocket/save/SaveSessionCoordinator.hpp"
#include <memory>

namespace Pocket::Emulator {
class MelonDsEngine final : public LibretroEngineBase {
public:
    explicit MelonDsEngine(const std::string& coreLibraryPath = "",
                           std::shared_ptr<Pocket::Save::SaveSessionCoordinator> coordinator =
                               std::make_shared<Pocket::Save::SaveSessionCoordinator>());
    explicit MelonDsEngine(std::shared_ptr<Pocket::Save::SaveSessionCoordinator> coordinator)
        : MelonDsEngine("", std::move(coordinator)) {}
    ~MelonDsEngine() override;
    bool loadRom(const std::string& romPath) override;
    void stop() override;
    const uint8_t* topFramebuffer() const;
    const uint8_t* bottomFramebuffer() const;
    size_t screenFramebufferSize() const;
    void sendTouchInput(int x, int y, bool isPressed);
    void getTouchInput(int& x, int& y, bool& isPressed) const;
    std::string loadedRomPath() const { return m_romPath; }
    std::string saveFilePath() const { return m_savePath; }
    int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id) override;

protected:
    void onFrameReceived(const uint8_t* pixels, unsigned width, unsigned height, size_t pitch) override;
    void afterGameLoaded() override;

private:
    std::shared_ptr<Pocket::Save::SaveSessionCoordinator> m_saveCoordinator;
    std::string m_savePath;
    std::vector<uint8_t> m_topFramebuffer, m_bottomFramebuffer;
    unsigned m_frameWidth{0}, m_frameHeight{0};
    int m_touchX{0}, m_touchY{0};
    bool m_touchPressed{false};
};
} // namespace Pocket::Emulator
