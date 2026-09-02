#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include "pocket/emulator/LibretroEngineBase.hpp"
#include "pocket/emulator/LibretroCoreSession.hpp"
#include <QDebug>
#include <QFileInfo>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <type_traits>

namespace Pocket::Emulator {
namespace {
void videoCallback(const void* d, unsigned w, unsigned h, size_t p) {
    if (auto* engine = LibretroCoreSession::current())
        engine->onVideoFrame(d, w, h, p);
}
size_t audioBatchCallback(const int16_t* d, size_t f) {
    if (auto* engine = LibretroCoreSession::current())
        return engine->onAudioSampleBatch(d, f);
    return f;
}
void audioSampleCallback(int16_t l, int16_t r) {
    const int16_t s[] = {l, r};
    if (auto* engine = LibretroCoreSession::current())
        engine->onAudioSampleBatch(s, 1);
}
void inputPollCallback() {}
bool environmentCallback(unsigned c, void* d) {
    auto* engine = LibretroCoreSession::current();
    return engine && engine->handleEnvironment(c, d);
}
int16_t inputCallback(unsigned p, unsigned d, unsigned i, unsigned id) {
    if (auto* engine = LibretroCoreSession::current())
        return engine->onInputState(p, d, i, id);
    return 0;
}
struct RetroLogCallback {
    void (*log)(int, const char*, ...);
};
void retroLog(int, const char* fmt, ...) {
    if (fmt)
        qDebug().noquote() << QString::fromUtf8(fmt);
}
} // namespace
LibretroEngineBase::LibretroEngineBase(const std::string& path) : m_coreLibraryPath(path) {
    m_buttonStates.fill(false);
    if (path.empty()) {
        m_coreError = "core library path is empty";
        return;
    }
    const QFileInfo file(QString::fromStdString(path));
    if (!file.exists()) {
        m_coreError = "core library not found: " + path;
        return;
    }
    if (!m_workDirectory.isValid()) {
        m_coreError = "could not create libretro working directory";
        return;
    }
    m_workDirectoryPath = m_workDirectory.path().toStdString();
    m_library.setFileName(file.absoluteFilePath());
    if (!m_library.load()) {
        m_coreError = "could not load core library: " + m_library.errorString().toStdString();
        return;
    }
    if (!resolveSymbols()) {
        m_library.unload();
        return;
    }
    retro_system_info info{};
    m_retro_get_system_info(&info);
    m_systemInfo = {info.library_name ? info.library_name : "", info.library_version ? info.library_version : "",
                    info.valid_extensions ? info.valid_extensions : "", info.need_fullpath, info.block_extract};
    if (!LibretroCoreSession::acquire(this)) {
        auto* owner = LibretroCoreSession::current();
        const std::string ownerName = owner ? owner->systemInfo().libraryName : "unknown";
        m_coreError = "another libretro core is already active (" + ownerName + ")";
        m_library.unload();
        return;
    }
    m_retro_set_environment(environmentCallback);
    m_retro_init();
    m_retro_set_video_refresh(videoCallback);
    m_retro_set_audio_sample(audioSampleCallback);
    m_retro_set_audio_sample_batch(audioBatchCallback);
    m_retro_set_input_poll(inputPollCallback);
    m_retro_set_input_state(inputCallback);
    m_hasCore = true;
}
LibretroEngineBase::~LibretroEngineBase() {
    stop();
    if (m_hasCore && m_retro_deinit) {
        m_retro_deinit();
    }
    if (m_library.isLoaded())
        m_library.unload();
    LibretroCoreSession::release(this);
}
bool LibretroEngineBase::resolveSymbols() {
    auto r = [this](auto& f, const char* n) {
        f = reinterpret_cast<std::remove_reference_t<decltype(f)>>(m_library.resolve(n));
        if (!f && m_coreError.empty())
            m_coreError = std::string("missing symbol ") + n;
    };
    r(m_retro_get_system_info, "retro_get_system_info");
    r(m_retro_set_environment, "retro_set_environment");
    r(m_retro_init, "retro_init");
    r(m_retro_deinit, "retro_deinit");
    r(m_retro_load_game, "retro_load_game");
    r(m_retro_unload_game, "retro_unload_game");
    r(m_retro_run, "retro_run");
    r(m_retro_get_memory_data, "retro_get_memory_data");
    r(m_retro_get_memory_size, "retro_get_memory_size");
    r(m_retro_set_video_refresh, "retro_set_video_refresh");
    r(m_retro_set_audio_sample, "retro_set_audio_sample");
    r(m_retro_set_audio_sample_batch, "retro_set_audio_sample_batch");
    r(m_retro_set_input_poll, "retro_set_input_poll");
    r(m_retro_set_input_state, "retro_set_input_state");
    r(m_retro_get_system_av_info, "retro_get_system_av_info");
    r(m_retro_set_controller_port_device, "retro_set_controller_port_device");
    return m_coreError.empty();
}
bool LibretroEngineBase::loadRom(const std::string& path) {
    if (!m_hasCore || m_gameLoaded)
        return false;
    m_romPath = path;
    retro_game_info info{};
    if (!m_systemInfo.needFullpath) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open())
            return false;
        const std::streamsize n = f.tellg();
        if (n <= 0)
            return false;
        f.seekg(0);
        m_romBuffer.resize(static_cast<size_t>(n));
        if (!f.read(reinterpret_cast<char*>(m_romBuffer.data()), n))
            return false;
    }
    info = buildGameInfo(m_romPath.c_str(), m_romBuffer.data(), m_romBuffer.size(), m_systemInfo.needFullpath);
    const bool loaded = m_retro_load_game(&info);
    if (!loaded)
        return false;
    m_gameLoaded = true;
    retro_system_av_info av{};
    m_retro_get_system_av_info(&av);
    if (av.timing.fps > 0)
        m_fps = av.timing.fps;
    if (av.timing.sample_rate > 0)
        m_sampleRate = av.timing.sample_rate;
    {
        std::lock_guard<std::mutex> lock(m_audioQueueMutex);
        if (m_audioQueueEnabled)
            m_audioQueue = std::make_unique<AudioRingBuffer>(static_cast<size_t>(std::ceil(m_sampleRate / 10.0)));
    }
    m_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    if (!m_sramBuffer.empty()) {
        PersistentGameSave s;
        s.setData(m_sramBuffer);
        m_sramBuffer.clear();
        loadPersistentSave(s);
    }
    afterGameLoaded();
    return true;
}
void LibretroEngineBase::start() {
    if (!m_hasCore || !m_gameLoaded || m_running)
        return;
    m_running = true;
    m_paused = false;
    m_executionThread = std::thread(&LibretroEngineBase::executionLoop, this);
}
void LibretroEngineBase::pause() {
    m_paused = true;
}
void LibretroEngineBase::resume() {
    m_paused = false;
}
void LibretroEngineBase::stop() {
    m_running = false;
    if (m_executionThread.joinable())
        m_executionThread.join();
    if (m_gameLoaded && m_retro_unload_game) {
        // Snapshot save RAM before the core lets go of it: unloading makes it
        // unreachable, so a caller that stops first would silently lose the
        // session. getPersistentSave() falls back to this buffer afterwards.
        if (m_retro_get_memory_data && m_retro_get_memory_size) {
            const void* data = m_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
            const size_t size = m_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
            if (data && size > 0) {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_sramBuffer.resize(size);
                std::memcpy(m_sramBuffer.data(), data, size);
            }
        }
        m_retro_unload_game();
        m_gameLoaded = false;
    }
}
void LibretroEngineBase::runFrameUnpaced() {
    if (m_gameLoaded && !m_running) {
        m_retro_run();
    }
}
void LibretroEngineBase::sendButtonEvent(EmulatorButton b, bool p) {
    std::lock_guard<std::mutex> l(m_stateMutex);
    const int i = (int)b;
    if (i >= 0 && i < (int)m_buttonStates.size())
        m_buttonStates[i] = p;
}
PersistentGameSave LibretroEngineBase::getPersistentSave() const {
    std::lock_guard<std::mutex> l(m_stateMutex);
    PersistentGameSave s;
    if (m_gameLoaded && m_retro_get_memory_data && m_retro_get_memory_size) {
        void* d = m_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        const size_t n = m_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if (d && n) {
            std::vector<uint8_t> b(n);
            std::memcpy(b.data(), d, n);
            s.setData(b);
            return s;
        }
    }
    s.setData(m_sramBuffer);
    return s;
}
bool LibretroEngineBase::loadPersistentSave(const PersistentGameSave& s) {
    if (s.isEmpty())
        return false;
    std::lock_guard<std::mutex> l(m_stateMutex);
    if (m_gameLoaded && m_retro_get_memory_data && m_retro_get_memory_size) {
        void* d = m_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        const size_t n = m_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if (d && n) {
            std::memcpy(d, s.data().data(), std::min(n, s.size()));
            return true;
        }
    }
    m_sramBuffer = s.data();
    return true;
}
void LibretroEngineBase::onFrameReceived(const uint8_t*, unsigned, unsigned, size_t) {}
size_t LibretroEngineBase::onAudioSampleBatch(const int16_t* d, size_t f) {
    std::lock_guard<std::mutex> lock(m_audioQueueMutex);
    if (m_audioQueueEnabled && m_audioQueue) {
        m_audioQueue->push(d, f);
        return f;
    }
    if (m_audioCallback && d && f)
        m_audioCallback(d, f);
    return f;
}
void LibretroEngineBase::setAudioQueueEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_audioQueueMutex);
    m_audioQueueEnabled = enabled;
    if (enabled && !m_audioQueue)
        m_audioQueue = std::make_unique<AudioRingBuffer>(static_cast<size_t>(std::ceil(m_sampleRate / 10.0)));
    if (!enabled)
        m_audioQueue.reset();
}
AudioRingBuffer* LibretroEngineBase::audioQueue() {
    std::lock_guard<std::mutex> lock(m_audioQueueMutex);
    return m_audioQueue.get();
}
retro_game_info LibretroEngineBase::buildGameInfo(const char* path, const void* data, size_t size, bool needFullpath) {
    retro_game_info info{};
    info.path = path;
    if (!needFullpath) {
        info.data = data;
        info.size = size;
    }
    return info;
}
void LibretroEngineBase::onVideoFrame(const void* d, unsigned w, unsigned h, size_t pitch) {
    if (!d || !w || !h)
        return;
    if (m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565 || m_pixelFormat == RETRO_PIXEL_FORMAT_0RGB1555) {
        std::vector<uint8_t> b((size_t)w * h * 4);
        const auto* src = static_cast<const uint8_t*>(d);
        for (unsigned y = 0; y < h; ++y) {
            const auto* row = reinterpret_cast<const uint16_t*>(src + (size_t)y * pitch);
            auto* out = reinterpret_cast<uint32_t*>(b.data() + (size_t)y * w * 4);
            for (unsigned x = 0; x < w; ++x) {
                const uint16_t p = row[x];
                const uint32_t r = ((p >> 11) & 31) << 3,
                               g = (m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565 ? ((p >> 5) & 63) << 2
                                                                               : ((p >> 5) & 31) << 3),
                               bl = (p & 31) << 3;
                out[x] = (r << 16) | (g << 8) | bl;
            }
        }
        onFrameReceived(b.data(), w, h, (size_t)w * 4);
        if (m_videoCallback)
            m_videoCallback(b.data(), w, h, (size_t)w * 4);
    } else {
        auto* p = static_cast<const uint8_t*>(d);
        onFrameReceived(p, w, h, pitch);
        if (m_videoCallback)
            m_videoCallback(p, w, h, pitch);
    }
}
int16_t LibretroEngineBase::onInputState(unsigned, unsigned device, unsigned, unsigned id) {
    if (device != RETRO_DEVICE_JOYPAD)
        return 0;
    std::lock_guard<std::mutex> l(m_stateMutex);
    int i = -1;
    switch (id) {
    case RETRO_DEVICE_ID_JOYPAD_UP:
        i = (int)EmulatorButton::Up;
        break;
    case RETRO_DEVICE_ID_JOYPAD_DOWN:
        i = (int)EmulatorButton::Down;
        break;
    case RETRO_DEVICE_ID_JOYPAD_LEFT:
        i = (int)EmulatorButton::Left;
        break;
    case RETRO_DEVICE_ID_JOYPAD_RIGHT:
        i = (int)EmulatorButton::Right;
        break;
    case RETRO_DEVICE_ID_JOYPAD_A:
        i = (int)EmulatorButton::A;
        break;
    case RETRO_DEVICE_ID_JOYPAD_B:
        i = (int)EmulatorButton::B;
        break;
    case RETRO_DEVICE_ID_JOYPAD_X:
        i = (int)EmulatorButton::X;
        break;
    case RETRO_DEVICE_ID_JOYPAD_Y:
        i = (int)EmulatorButton::Y;
        break;
    case RETRO_DEVICE_ID_JOYPAD_L:
        i = (int)EmulatorButton::L;
        break;
    case RETRO_DEVICE_ID_JOYPAD_R:
        i = (int)EmulatorButton::R;
        break;
    case RETRO_DEVICE_ID_JOYPAD_START:
        i = (int)EmulatorButton::Start;
        break;
    case RETRO_DEVICE_ID_JOYPAD_SELECT:
        i = (int)EmulatorButton::Select;
        break;
    default:
        break;
    }
    return i >= 0 && m_buttonStates[i] ? 1 : 0;
}
bool LibretroEngineBase::handleEnvironment(unsigned c, void* d) {
    switch (c) {
    // SUPPORTED: Pocket supplies the requested data or applies the requested setting.
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        if (!d)
            return false;
        if (*static_cast<const retro_pixel_format*>(d) != RETRO_PIXEL_FORMAT_XRGB8888 &&
            *static_cast<const retro_pixel_format*>(d) != RETRO_PIXEL_FORMAT_RGB565 &&
            *static_cast<const retro_pixel_format*>(d) != RETRO_PIXEL_FORMAT_0RGB1555)
            return false;
        m_pixelFormat = *static_cast<const retro_pixel_format*>(d);
        return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (!d)
            return false;
        *static_cast<const char**>(d) = m_workDirectoryPath.c_str();
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        if (!d)
            return false;
        *static_cast<bool*>(d) = true;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        if (d)
            static_cast<retro_variable*>(d)->value = nullptr;
        return false;
    // SAFE_NOOP: accepted metadata which does not need runtime action in Pocket.
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        return true;
    // SUPPORTED: Pocket supplies a stable value to the core.
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (!d)
            return false;
        *static_cast<bool*>(d) = false;
        return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
    case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
        if (!d)
            return false;
        *static_cast<unsigned*>(d) = 0;
        return true;
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        if (!d)
            return false;
        static_cast<RetroLogCallback*>(d)->log = retroLog;
        return true;
    default:
        if (handleEnvironmentExtra(c, d))
            return true;
        // UNSUPPORTED: report each command once; GET_VARIABLE_UPDATE is intentionally silent above.
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_unknownEnvironmentCommands.insert(c).second)
                qDebug() << "Unsupported libretro environment command:" << c;
        }
        return false;
    }
}
void LibretroEngineBase::executionLoop() {
    using clock = std::chrono::steady_clock;
    const auto interval = std::chrono::microseconds((long long)(1000000.0 / m_fps));
    auto next = clock::now();
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    while (m_running) {
        if (!m_paused)
            m_retro_run();
        next += interval;
        auto now = clock::now();
        if (now > next) {
            next = now;
            continue;
        }
        constexpr auto spin = std::chrono::microseconds(1500);
        const auto remaining = next - now;
        if (remaining > spin)
            std::this_thread::sleep_for(remaining - spin);
        while (m_running && clock::now() < next)
            std::this_thread::yield();
    }
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}
} // namespace Pocket::Emulator
