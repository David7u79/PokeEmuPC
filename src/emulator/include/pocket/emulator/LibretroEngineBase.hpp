#pragma once

#include "pocket/emulator/EmulatorEngine.hpp"
#include "pocket/emulator/Ilibretro.h"
#include <QLibrary>
#include <QTemporaryDir>
#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Pocket::Emulator {
struct LibretroSystemInfo {
    std::string libraryName, libraryVersion, validExtensions;
    bool needFullpath{false};
    bool blockExtract{false};
};
class LibretroEngineBase : public EmulatorEngine {
public:
    explicit LibretroEngineBase(const std::string& coreLibraryPath);
    ~LibretroEngineBase() override;
    bool loadRom(const std::string& romPath) override;
    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool isPaused() const override { return m_paused; }
    bool hasCore() const { return m_hasCore; }
    std::string coreError() const { return m_coreError; }
    const LibretroSystemInfo& systemInfo() const { return m_systemInfo; }
    double fps() const { return m_fps; }
    double sampleRate() const { return m_sampleRate; }
    void runFrameUnpaced();
    void sendButtonEvent(EmulatorButton button, bool pressed) override;
    PersistentGameSave getPersistentSave() const override;
    bool loadPersistentSave(const PersistentGameSave& save) override;
    void setVideoFrameCallback(VideoFrameCallback callback) override { m_videoCallback = std::move(callback); }
    void setAudioSampleCallback(AudioSampleCallback callback) override { m_audioCallback = std::move(callback); }
    void onVideoFrame(const void* data, unsigned width, unsigned height, size_t pitch);
    size_t onAudioSampleBatch(const int16_t* data, size_t frames);
    virtual int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id);
    bool handleEnvironment(unsigned cmd, void* data);

protected:
    virtual bool handleEnvironmentExtra(unsigned cmd, void* data) {
        (void)cmd;
        (void)data;
        return false;
    }
    virtual void onFrameReceived(const uint8_t* pixels, unsigned width, unsigned height, size_t pitch);
    virtual void afterGameLoaded() {}
    std::atomic<int> m_pixelFormat{RETRO_PIXEL_FORMAT_XRGB8888};
    std::array<bool, 16> m_buttonStates{};
    mutable std::mutex m_stateMutex;
    std::string m_romPath;
    bool m_gameLoaded{false};
    void (*m_retro_run)(void){nullptr};

private:
    void executionLoop();
    bool resolveSymbols();
    void activateCallbackContext() const;
    void deactivateCallbackContext() const;
    std::string m_coreLibraryPath, m_coreError, m_workDirectoryPath;
    QLibrary m_library;
    QTemporaryDir m_workDirectory;
    LibretroSystemInfo m_systemInfo;
    std::atomic<bool> m_running{false}, m_paused{false};
    bool m_hasCore{false};
    double m_fps{59.7275}, m_sampleRate{32768.0};
    std::thread m_executionThread;
    VideoFrameCallback m_videoCallback;
    AudioSampleCallback m_audioCallback;
    std::vector<uint8_t> m_sramBuffer, m_romBuffer;
    void (*m_retro_get_system_info)(retro_system_info*){nullptr};
    void (*m_retro_init)(void){nullptr};
    void (*m_retro_deinit)(void){nullptr};
    bool (*m_retro_load_game)(const retro_game_info*){nullptr};
    void (*m_retro_unload_game)(void){nullptr};
    void* (*m_retro_get_memory_data)(unsigned){nullptr};
    size_t (*m_retro_get_memory_size)(unsigned){nullptr};
    void (*m_retro_set_video_refresh)(retro_video_refresh_t){nullptr};
    void (*m_retro_set_environment)(retro_environment_t){nullptr};
    void (*m_retro_set_audio_sample)(retro_audio_sample_t){nullptr};
    void (*m_retro_set_audio_sample_batch)(retro_audio_sample_batch_t){nullptr};
    void (*m_retro_set_input_poll)(retro_input_poll_t){nullptr};
    void (*m_retro_set_input_state)(retro_input_state_t){nullptr};
    void (*m_retro_get_system_av_info)(retro_system_av_info*){nullptr};
    void (*m_retro_set_controller_port_device)(unsigned, unsigned){nullptr};
};
} // namespace Pocket::Emulator
