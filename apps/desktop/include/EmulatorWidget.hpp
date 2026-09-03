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
    Pocket::Emulator::LibretroEngineBase* engine() const { return m_engine.get(); }
    AudioSink& audioSink() { return m_audioSink; }
    QString savePath() const { return m_savePath; }

    // The bindings the user configured in Settings -> Controls.
    void setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    void setControllerSystem(const QString& system);
    void refreshKeyBindings();
    bool hintsVisible() const { return m_hintsVisible; }
    void setHintsVisible(bool visible);
    void toggleHints();
    EmulatorViewMode viewMode() const { return m_viewMode; }
    void setViewMode(EmulatorViewMode mode);
    void toggleViewMode();
    // Which emulator button a key drives, or nothing when it is unbound.
    std::optional<Pocket::Emulator::EmulatorButton> buttonForKey(int key) const;

signals:
    void mappingEdited();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

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
    QHash<int, QString> m_keyControlIds;
    ControllerHintOverlay m_hintOverlay;
    bool m_hintsVisible{true};
    EmulatorViewMode m_viewMode{EmulatorViewMode::ConsoleFrame};
    QString m_mousePressedControlId;
    QString m_capturingControlId;
    QTimer m_captureBlinkTimer;
    bool m_captureBlinkOn{true};

    AudioSink m_audioSink;

    void showControlMenu(const Pocket::Input::ControllerControl* control, const QPoint& globalPos);
    void releaseMouseControl();
    void saveMapping();
    void beginCapture(const QString& controlId);
    void applyCapturedBinding(int key);
};

} // namespace Pocket::App
