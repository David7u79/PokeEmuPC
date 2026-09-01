#include "pocket/emulator/MgbaEngine.hpp"
#include "pocket/core/GameSystem.hpp"
#include <QDebug>
#include <fstream>
#include <chrono>

namespace Pocket::Emulator {

static MgbaEngine* s_currentEngine = nullptr;

static void globalVideoCallback(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (s_currentEngine) {
        s_currentEngine->onVideoFrame(data, width, height, pitch);
    }
}

static size_t globalAudioCallback(const int16_t *data, size_t frames) {
    if (s_currentEngine) {
        return s_currentEngine->onAudioSampleBatch(data, frames);
    }
    return frames;
}

static int16_t globalInputCallback(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (s_currentEngine) {
        return s_currentEngine->onInputState(port, device, index, id);
    }
    return 0;
}

MgbaEngine::MgbaEngine(const std::string& coreLibraryPath)
    : m_coreLibraryPath(coreLibraryPath) {
    m_buttonStates.fill(false);

    if (!coreLibraryPath.empty()) {
        m_library.setFileName(QString::fromStdString(coreLibraryPath));
        if (m_library.load()) {
            m_retro_init = reinterpret_cast<decltype(m_retro_init)>(m_library.resolve("retro_init"));
            m_retro_deinit = reinterpret_cast<decltype(m_retro_deinit)>(m_library.resolve("retro_deinit"));
            m_retro_load_game = reinterpret_cast<decltype(m_retro_load_game)>(m_library.resolve("retro_load_game"));
            m_retro_unload_game = reinterpret_cast<decltype(m_retro_unload_game)>(m_library.resolve("retro_unload_game"));
            m_retro_run = reinterpret_cast<decltype(m_retro_run)>(m_library.resolve("retro_run"));
            m_retro_get_memory_data = reinterpret_cast<decltype(m_retro_get_memory_data)>(m_library.resolve("retro_get_memory_data"));
            m_retro_get_memory_size = reinterpret_cast<decltype(m_retro_get_memory_size)>(m_library.resolve("retro_get_memory_size"));
            m_retro_set_video_refresh = reinterpret_cast<decltype(m_retro_set_video_refresh)>(m_library.resolve("retro_set_video_refresh"));
            m_retro_set_audio_sample_batch = reinterpret_cast<decltype(m_retro_set_audio_sample_batch)>(m_library.resolve("retro_set_audio_sample_batch"));
            m_retro_set_input_state = reinterpret_cast<decltype(m_retro_set_input_state)>(m_library.resolve("retro_set_input_state"));

            if (m_retro_init) {
                m_retro_init();
            }
            if (m_retro_set_video_refresh) m_retro_set_video_refresh(globalVideoCallback);
            if (m_retro_set_audio_sample_batch) m_retro_set_audio_sample_batch(globalAudioCallback);
            if (m_retro_set_input_state) m_retro_set_input_state(globalInputCallback);
        }
    }
}

MgbaEngine::~MgbaEngine() {
    stop();
    if (m_retro_deinit) {
        m_retro_deinit();
    }
    if (m_library.isLoaded()) {
        m_library.unload(); // Cleanly unloads emulator core DLL from memory
    }
}

bool MgbaEngine::loadRom(const std::string& romPath) {
    m_romPath = romPath;
    std::ifstream file(romPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    if (size <= 0) return false;

    file.seekg(0, std::ios::beg);
    m_romBuffer.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_romBuffer.data()), size)) return false;

    if (m_retro_load_game) {
        struct retro_game_info info{};
        info.path = m_romPath.c_str();
        info.data = m_romBuffer.data();
        info.size = m_romBuffer.size();
        return m_retro_load_game(&info);
    }

    // Default 64KB SRAM allocation for testing if no external core DLL is loaded
    m_sramBuffer.resize(65536, 0xFF);
    return true;
}

void MgbaEngine::start() {
    if (m_running) return;
    s_currentEngine = this;
    m_running = true;
    m_paused = false;
    m_executionThread = std::thread(&MgbaEngine::executionLoop, this);
}

void MgbaEngine::pause() {
    m_paused = true;
}

void MgbaEngine::resume() {
    m_paused = false;
}

void MgbaEngine::stop() {
    m_running = false;
    if (m_executionThread.joinable()) {
        m_executionThread.join();
    }
    if (s_currentEngine == this) {
        s_currentEngine = nullptr;
    }
    if (m_retro_unload_game) {
        m_retro_unload_game();
    }
}

void MgbaEngine::sendButtonEvent(EmulatorButton button, bool pressed) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    int buttonIdx = static_cast<int>(button);
    if (buttonIdx >= 0 && buttonIdx < static_cast<int>(m_buttonStates.size())) {
        m_buttonStates[buttonIdx] = pressed;
    }
}

PersistentGameSave MgbaEngine::getPersistentSave() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    PersistentGameSave save;

    if (m_retro_get_memory_data && m_retro_get_memory_size) {
        void* data = m_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        size_t size = m_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if (data && size > 0) {
            std::vector<uint8_t> sram(size);
            std::memcpy(sram.data(), data, size);
            save.setData(sram);
            return save;
        }
    }

    save.setData(m_sramBuffer);
    return save;
}

bool MgbaEngine::loadPersistentSave(const PersistentGameSave& save) {
    if (save.isEmpty()) return false; // 12-Step Save Safety

    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_retro_get_memory_data && m_retro_get_memory_size) {
        void* data = m_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        size_t size = m_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if (data && size > 0) {
            size_t copySize = std::min(size, save.size());
            std::memcpy(data, save.data().data(), copySize);
            return true;
        }
    }

    m_sramBuffer = save.data();
    return true;
}

void MgbaEngine::onVideoFrame(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (m_videoCallback && data && width > 0 && height > 0) {
        m_videoCallback(reinterpret_cast<const uint8_t*>(data), static_cast<int>(width), static_cast<int>(height), pitch);
    }
}

size_t MgbaEngine::onAudioSampleBatch(const int16_t *data, size_t frames) {
    if (m_audioCallback && data && frames > 0) {
        m_audioCallback(data, frames);
    }
    return frames;
}

int16_t MgbaEngine::onInputState(unsigned, unsigned, unsigned, unsigned id) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    int mappedId = -1;
    switch (id) {
        case RETRO_DEVICE_ID_JOYPAD_UP:     mappedId = static_cast<int>(EmulatorButton::Up); break;
        case RETRO_DEVICE_ID_JOYPAD_DOWN:   mappedId = static_cast<int>(EmulatorButton::Down); break;
        case RETRO_DEVICE_ID_JOYPAD_LEFT:   mappedId = static_cast<int>(EmulatorButton::Left); break;
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:  mappedId = static_cast<int>(EmulatorButton::Right); break;
        case RETRO_DEVICE_ID_JOYPAD_A:      mappedId = static_cast<int>(EmulatorButton::A); break;
        case RETRO_DEVICE_ID_JOYPAD_B:      mappedId = static_cast<int>(EmulatorButton::B); break;
        case RETRO_DEVICE_ID_JOYPAD_L:      mappedId = static_cast<int>(EmulatorButton::L); break;
        case RETRO_DEVICE_ID_JOYPAD_R:      mappedId = static_cast<int>(EmulatorButton::R); break;
        case RETRO_DEVICE_ID_JOYPAD_START:  mappedId = static_cast<int>(EmulatorButton::Start); break;
        case RETRO_DEVICE_ID_JOYPAD_SELECT: mappedId = static_cast<int>(EmulatorButton::Select); break;
        default: break;
    }

    if (mappedId >= 0 && mappedId < static_cast<int>(m_buttonStates.size())) {
        return m_buttonStates[mappedId] ? 1 : 0;
    }
    return 0;
}

void MgbaEngine::executionLoop() {
    using clock = std::chrono::steady_clock;
    auto targetInterval = std::chrono::microseconds(16666); // ~60 FPS

    // Extension-based system resolution calculation (GB/GBC: 160x144, GBA: 240x160)
    auto systemOpt = Pocket::Core::GameSystemUtils::detectFromExtension(m_romPath);
    Pocket::Core::GameSystem system = systemOpt.value_or(Pocket::Core::GameSystem::GBA);

    int width = (system == Pocket::Core::GameSystem::GBA) ? 240 : 160;
    int height = (system == Pocket::Core::GameSystem::GBA) ? 160 : 144;

    std::vector<uint8_t> dummyFrame(static_cast<size_t>(width * height * 4), 0x1F);

    while (m_running) {
        auto startTime = clock::now();

        if (!m_paused) {
            if (m_retro_run) {
                m_retro_run();
            } else {
                // Fallback frame renderer for test environment without core DLL
                if (m_videoCallback) {
                    m_videoCallback(dummyFrame.data(), width, height, static_cast<size_t>(width * 4));
                }
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - startTime);
        if (elapsed < targetInterval) {
            std::this_thread::sleep_for(targetInterval - elapsed);
        }
    }
}

} // namespace Pocket::Emulator
