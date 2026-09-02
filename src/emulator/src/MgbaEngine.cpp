#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

#include "pocket/emulator/MgbaEngine.hpp"
#include "pocket/core/GameSystem.hpp"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <fstream>
#include <chrono>
#include <cstring>
#include <type_traits>

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

static void globalAudioSampleCallback(int16_t left, int16_t right) {
    const int16_t sample[] = {left, right};
    if (s_currentEngine) {
        s_currentEngine->onAudioSampleBatch(sample, 1);
    }
}

static void globalInputPollCallback() {}

static bool globalEnvironmentCallback(unsigned cmd, void* data) {
    return s_currentEngine ? s_currentEngine->handleEnvironment(cmd, data) : false;
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
    if (coreLibraryPath.empty()) {
        m_coreError = "core library path is empty";
        return;
    }

    const QFileInfo coreFile(QString::fromStdString(coreLibraryPath));
    if (!coreFile.exists()) {
        m_coreError = "core library not found: " + coreLibraryPath;
        return;
    }
    m_coreDirectory = coreFile.absolutePath().toStdString();
    m_library.setFileName(coreFile.absoluteFilePath());
    if (!m_library.load()) {
        m_coreError = "could not load core library: " + m_library.errorString().toStdString();
        return;
    }

    auto resolve = [this](auto& function, const char* name) {
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(m_library.resolve(name));
        if (!function && m_coreError.empty()) {
            m_coreError = std::string("missing symbol ") + name;
        }
    };
    resolve(m_retro_set_environment, "retro_set_environment");
    resolve(m_retro_init, "retro_init");
    resolve(m_retro_deinit, "retro_deinit");
    resolve(m_retro_load_game, "retro_load_game");
    resolve(m_retro_unload_game, "retro_unload_game");
    resolve(m_retro_run, "retro_run");
    resolve(m_retro_get_memory_data, "retro_get_memory_data");
    resolve(m_retro_get_memory_size, "retro_get_memory_size");
    resolve(m_retro_set_video_refresh, "retro_set_video_refresh");
    resolve(m_retro_set_audio_sample, "retro_set_audio_sample");
    resolve(m_retro_set_audio_sample_batch, "retro_set_audio_sample_batch");
    resolve(m_retro_set_input_poll, "retro_set_input_poll");
    resolve(m_retro_set_input_state, "retro_set_input_state");
    resolve(m_retro_get_system_av_info, "retro_get_system_av_info");
    resolve(m_retro_set_controller_port_device, "retro_set_controller_port_device");
    if (!m_coreError.empty()) {
        m_library.unload();
        return;
    }

    s_currentEngine = this;
    m_retro_set_environment(globalEnvironmentCallback);
    m_retro_init();
    m_retro_set_video_refresh(globalVideoCallback);
    m_retro_set_audio_sample(globalAudioSampleCallback);
    m_retro_set_audio_sample_batch(globalAudioCallback);
    m_retro_set_input_poll(globalInputPollCallback);
    m_retro_set_input_state(globalInputCallback);
    m_hasCore = true;
}

MgbaEngine::~MgbaEngine() {
    stop();
    if (m_hasCore && m_retro_deinit) {
        m_retro_deinit();
    }
    if (m_library.isLoaded()) {
        m_library.unload(); // Cleanly unloads emulator core DLL from memory
    }
}

bool MgbaEngine::loadRom(const std::string& romPath) {
    if (!m_hasCore) return false;
    m_romPath = romPath;
    std::ifstream file(romPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    if (size <= 0) return false;

    file.seekg(0, std::ios::beg);
    m_romBuffer.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_romBuffer.data()), size)) return false;

    retro_game_info info{};
    info.path = m_romPath.c_str();
    info.data = m_romBuffer.data();
    info.size = m_romBuffer.size();
    if (!m_retro_load_game(&info)) return false;
    m_gameLoaded = true;
    retro_system_av_info avInfo{};
    m_retro_get_system_av_info(&avInfo);
    if (avInfo.timing.fps > 0.0) m_fps = avInfo.timing.fps;
    m_sampleRate = avInfo.timing.sample_rate > 0.0 ? avInfo.timing.sample_rate : 32768.0;
    m_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

    // A save handed over before the game was loaded was only staged; the core
    // exposes its save RAM now, so apply it.
    if (!m_sramBuffer.empty()) {
        PersistentGameSave staged;
        staged.setData(m_sramBuffer);
        m_sramBuffer.clear();
        loadPersistentSave(staged);
    }
    return true;
}

void MgbaEngine::start() {
    if (!m_hasCore || !m_gameLoaded || m_running) return;
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
    if (m_gameLoaded && m_retro_unload_game) {
        m_retro_unload_game();
        m_gameLoaded = false;
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

    if (m_gameLoaded && m_retro_get_memory_data && m_retro_get_memory_size) {
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
    // Querying save RAM before retro_load_game dereferences a null core inside
    // mGBA and takes the whole process down.
    if (m_gameLoaded && m_retro_get_memory_data && m_retro_get_memory_size) {
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
        if (m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565 || m_pixelFormat == RETRO_PIXEL_FORMAT_0RGB1555) {
            std::vector<uint8_t> converted(static_cast<size_t>(width) * height * 4U);
            const auto* source = reinterpret_cast<const uint8_t*>(data);
            for (unsigned y = 0; y < height; ++y) {
                const auto* row = reinterpret_cast<const uint16_t*>(source + static_cast<size_t>(y) * pitch);
                auto* target = reinterpret_cast<uint32_t*>(converted.data() + static_cast<size_t>(y) * width * 4U);
                for (unsigned x = 0; x < width; ++x) {
                    const uint16_t pixel = row[x];
                    const uint32_t r = ((pixel >> 11U) & 0x1fU) << 3U;
                    const uint32_t g = m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565
                        ? ((pixel >> 5U) & 0x3fU) << 2U
                        : ((pixel >> 5U) & 0x1fU) << 3U;
                    const uint32_t b = (pixel & 0x1fU) << 3U;
                    target[x] = (r << 16U) | (g << 8U) | b;
                }
            }
            m_videoCallback(converted.data(), static_cast<int>(width), static_cast<int>(height), static_cast<size_t>(width) * 4U);
        } else {
            m_videoCallback(reinterpret_cast<const uint8_t*>(data), static_cast<int>(width), static_cast<int>(height), pitch);
        }
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
    const auto targetInterval = std::chrono::microseconds(static_cast<long long>(1000000.0 / m_fps));
    auto next = clock::now();

#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    while (m_running) {
        if (!m_paused) {
            m_retro_run();
        }

        next += targetInterval;
        auto now = clock::now();
        if (now > next) {
            next = now;
            continue;
        }

        constexpr auto spinWindow = std::chrono::microseconds(1500);
        const auto remaining = next - now;
        if (remaining > spinWindow) {
            std::this_thread::sleep_for(remaining - spinWindow);
        }
        while (m_running && clock::now() < next) {
            std::this_thread::yield();
        }
    }

#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

bool MgbaEngine::handleEnvironment(unsigned cmd, void* data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        if (!data) return false;
        if (*static_cast<const retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888 ||
            *static_cast<const retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_RGB565 ||
            *static_cast<const retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_0RGB1555) {
            m_pixelFormat = *static_cast<const retro_pixel_format*>(data);
            return true;
        }
        return false;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (!data) return false;
        if (m_coreDirectory.empty()) m_coreDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString();
        *static_cast<const char**>(data) = m_coreDirectory.c_str();
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        if (!data) return false;
        *static_cast<bool*>(data) = true;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        if (!data) return false;
        static_cast<retro_variable*>(data)->value = nullptr;
        return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (!data) return false;
        *static_cast<bool*>(data) = false;
        return true;
    default:
        return false;
    }
}

} // namespace Pocket::Emulator
