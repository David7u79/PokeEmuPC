#pragma once

#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include <thread>
#include <atomic>
#include <QLibrary>
#include "pocket/emulator/EmulatorEngine.hpp"
#include "pocket/emulator/Ilibretro.h"

namespace Pocket::Emulator {

class MgbaEngine : public EmulatorEngine {
public:
    explicit MgbaEngine(const std::string& coreLibraryPath = "");
    ~MgbaEngine() override;

    bool loadRom(const std::string& romPath) override;
    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    bool isRunning() const override { return m_running; }
    bool isPaused() const override { return m_paused; }
    bool hasCore() const { return m_hasCore; }
    std::string coreError() const { return m_coreError; }
    double sampleRate() const { return m_sampleRate; }

    void sendButtonEvent(EmulatorButton button, bool pressed) override;

    PersistentGameSave getPersistentSave() const override;
    bool loadPersistentSave(const PersistentGameSave& save) override;

    void setVideoFrameCallback(VideoFrameCallback callback) override { m_videoCallback = std::move(callback); }
    void setAudioSampleCallback(AudioSampleCallback callback) override { m_audioCallback = std::move(callback); }

    // Libretro C callbacks
    void onVideoFrame(const void* data, unsigned width, unsigned height, size_t pitch);
    size_t onAudioSampleBatch(const int16_t* data, size_t frames);
    int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id);
    bool handleEnvironment(unsigned cmd, void* data);

private:
    void executionLoop();

    std::string m_coreLibraryPath;
    std::string m_romPath;
    QLibrary m_library;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    bool m_hasCore{false};
    bool m_gameLoaded{false};
    std::string m_coreError;
    std::string m_coreDirectory;
    std::atomic<int> m_pixelFormat{RETRO_PIXEL_FORMAT_XRGB8888};
    double m_fps{59.7275};
    double m_sampleRate{32768.0};
    std::thread m_executionThread;

    VideoFrameCallback m_videoCallback;
    AudioSampleCallback m_audioCallback;

    std::array<bool, 16> m_buttonStates{};
    mutable std::mutex m_stateMutex;

    std::vector<uint8_t> m_sramBuffer;
    std::vector<uint8_t> m_romBuffer;

    // Direct C callbacks pointers if libretro DLL is loaded
    void (*m_retro_init)(void){nullptr};
    void (*m_retro_deinit)(void){nullptr};
    bool (*m_retro_load_game)(const struct retro_game_info *game){nullptr};
    void (*m_retro_unload_game)(void){nullptr};
    void (*m_retro_run)(void){nullptr};
    void* (*m_retro_get_memory_data)(unsigned id){nullptr};
    size_t (*m_retro_get_memory_size)(unsigned id){nullptr};
    void (*m_retro_set_video_refresh)(retro_video_refresh_t){nullptr};
    void (*m_retro_set_environment)(retro_environment_t){nullptr};
    void (*m_retro_set_audio_sample)(retro_audio_sample_t){nullptr};
    void (*m_retro_set_audio_sample_batch)(retro_audio_sample_batch_t){nullptr};
    void (*m_retro_set_input_poll)(retro_input_poll_t){nullptr};
    void (*m_retro_set_input_state)(retro_input_state_t){nullptr};
    void (*m_retro_get_system_av_info)(retro_system_av_info*){nullptr};
    void (*m_retro_set_controller_port_device)(unsigned port, unsigned device){nullptr};
};

} // namespace Pocket::Emulator
