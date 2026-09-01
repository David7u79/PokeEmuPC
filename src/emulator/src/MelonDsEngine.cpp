#include "pocket/emulator/MelonDsEngine.hpp"
#include <fstream>
#include <algorithm>

namespace Pocket::Emulator {

MelonDsEngine::MelonDsEngine(std::shared_ptr<Pocket::Save::SaveSessionCoordinator> coordinator)
    : m_saveCoordinator(std::move(coordinator)) {}

MelonDsEngine::~MelonDsEngine() {
    stop();
}

bool MelonDsEngine::loadRom(const std::string& romPath) {
    if (m_isLoaded) {
        stop();
    }

    std::ifstream file(romPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.close();

    m_romPath = romPath;
    m_savePath = romPath + ".sav";

    // Acquire Save Ownership Lock
    if (!m_saveCoordinator->acquireEmulatorLock(m_savePath)) {
        return false;
    }

    // Allocate Dual 256x192 RGBA Framebuffers
    m_topFramebuffer.assign(kScreenSizeBytes, 0x00);
    m_bottomFramebuffer.assign(kScreenSizeBytes, 0x00);

    // Synthetic Initial Pattern for Top Screen (Navy Blue) & Bottom Screen (Dark Cyan)
    for (size_t i = 0; i < kScreenSizeBytes; i += 4) {
        m_topFramebuffer[i]     = 0x1A; // R
        m_topFramebuffer[i + 1] = 0x25; // G
        m_topFramebuffer[i + 2] = 0x36; // B
        m_topFramebuffer[i + 3] = 0xFF; // A

        m_bottomFramebuffer[i]     = 0x2E; // R
        m_bottomFramebuffer[i + 1] = 0x3A; // G
        m_bottomFramebuffer[i + 2] = 0x46; // B
        m_bottomFramebuffer[i + 3] = 0xFF; // A
    }

    m_isLoaded = true;
    m_isRunning = false;
    m_isPaused = false;
    return true;
}

void MelonDsEngine::start() {
    if (!m_isLoaded) return;
    m_isRunning = true;
    m_isPaused = false;
}

void MelonDsEngine::pause() {
    if (!m_isRunning) return;
    m_isPaused = true;
}

void MelonDsEngine::resume() {
    if (!m_isPaused) return;
    m_isPaused = false;
}

void MelonDsEngine::stop() {
    if (!m_isLoaded) return;

    // Release Save Ownership Lock & Flush
    if (!m_savePath.empty()) {
        m_saveCoordinator->releaseEmulatorLock(m_savePath);
    }

    // Release Framebuffer Memory
    std::vector<uint8_t>().swap(m_topFramebuffer);
    std::vector<uint8_t>().swap(m_bottomFramebuffer);

    m_romPath.clear();
    m_savePath.clear();
    m_isLoaded = false;
    m_isRunning = false;
    m_isPaused = false;
}

void MelonDsEngine::sendButtonEvent(EmulatorButton button, bool pressed) {
    (void)button;
    (void)pressed;
}

PersistentGameSave MelonDsEngine::getPersistentSave() const {
    PersistentGameSave save(m_savePath);
    save.loadFromFile(m_savePath);
    return save;
}

bool MelonDsEngine::loadPersistentSave(const PersistentGameSave& save) {
    if (save.savePath().empty()) return false;
    m_savePath = save.savePath();
    return true;
}

void MelonDsEngine::sendTouchInput(int x, int y, bool isPressed) {
    m_touchX = std::clamp(x, 0, kScreenWidth - 1);
    m_touchY = std::clamp(y, 0, kScreenHeight - 1);
    m_touchPressed = isPressed;
}

void MelonDsEngine::getTouchInput(int& x, int& y, bool& isPressed) const {
    x = m_touchX;
    y = m_touchY;
    isPressed = m_touchPressed;
}

} // namespace Pocket::Emulator
