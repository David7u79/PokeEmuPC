#pragma once

#include <QWidget>
#include <QImage>
#include <QKeyEvent>
#include <memory>
#include <mutex>

#include "AudioSink.hpp"
#include "ControllerHintOverlay.hpp"
#include "pocket/emulator/MgbaEngine.hpp"
#include "pocket/input/ControllerMapping.hpp"
#include <QHash>
#include <QTimer>

namespace Pocket::App {

class EmulatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EmulatorWidget(QWidget* parent = nullptr);
    ~EmulatorWidget() override;

    bool loadAndStartRom(const QString& romPath, const QString& savePath = "");
    void stopEmulator();
    void setCoreLibraryPath(const QString& path);
    void setStatusMessage(const QString& message);

    // The bindings the user configured in Settings -> Controls.
    void setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    void setControllerSystem(const QString& system);
    void refreshKeyBindings();
    bool hintsVisible() const { return m_hintsVisible; }
    void setHintsVisible(bool visible);
    void toggleHints();
    // Which emulator button a key drives, or nothing when it is unbound.
    std::optional<Pocket::Emulator::EmulatorButton> buttonForKey(int key) const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onFrameReady();

private:
    std::unique_ptr<Pocket::Emulator::MgbaEngine> m_engine;
    QImage m_currentFrame;
    std::mutex m_frameMutex;
    QString m_savePath;
    QString m_coreLibraryPath;
    QString m_statusMessage{"No ROM loaded"};
    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
    QString m_controllerSystem{"GBA"};
    QHash<int, Pocket::Emulator::EmulatorButton> m_keyBindings;
    ControllerHintOverlay m_hintOverlay;
    bool m_hintsVisible{true};

    AudioSink m_audioSink;
};

} // namespace Pocket::App
