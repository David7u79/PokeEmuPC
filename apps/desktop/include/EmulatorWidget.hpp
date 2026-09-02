#pragma once

#include <QWidget>
#include <QImage>
#include <QKeyEvent>
#include <memory>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include "pocket/emulator/MgbaEngine.hpp"
#include "pocket/input/ControllerMapping.hpp"
#include <QHash>

namespace Pocket::App {

class EmulatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EmulatorWidget(QWidget *parent = nullptr);
    ~EmulatorWidget() override;

    bool loadAndStartRom(const QString& romPath, const QString& savePath = "");
    void stopEmulator();
    void setCoreLibraryPath(const QString& path);
    void setStatusMessage(const QString& message);

    // The bindings the user configured in Settings -> Controls.
    void setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    void setControllerSystem(const QString& system);
    void refreshKeyBindings();
    // Which emulator button a key drives, or nothing when it is unbound.
    std::optional<Pocket::Emulator::EmulatorButton> buttonForKey(int key) const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onFrameReady();

private:
    void initAudio(int sampleRate);
    void writeAudioSamples(const int16_t* samples, size_t frames);
    void closeAudio();

    std::unique_ptr<Pocket::Emulator::MgbaEngine> m_engine;
    QImage m_currentFrame;
    std::mutex m_frameMutex;
    QString m_savePath;
    QString m_coreLibraryPath;
    QString m_statusMessage{"No ROM loaded"};
    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
    QString m_controllerSystem{"GBA"};
    QHash<int, Pocket::Emulator::EmulatorButton> m_keyBindings;

#ifdef _WIN32
    HWAVEOUT m_waveOut{nullptr};
    WAVEHDR m_waveHeaders[8]{};
    std::vector<int16_t> m_audioBuffers[8];
    size_t m_currentBufferIndex{0};
    bool m_audioInitialized{false};
#endif
};

} // namespace Pocket::App
