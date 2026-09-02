#include "EmulatorWidget.hpp"
#include "pocket/input/ControllerLayout.hpp"
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <cstring>

namespace Pocket::App {

EmulatorWidget::EmulatorWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(480, 320); // 2x scale GBA 240x160 resolution
    m_hintOverlay.setSystem(m_controllerSystem);
    m_hintTimer.setSingleShot(true);
    connect(&m_hintTimer, &QTimer::timeout, this, [this] { setHintsVisible(false); });
}

EmulatorWidget::~EmulatorWidget() {
    stopEmulator();
}

bool EmulatorWidget::loadAndStartRom(const QString& romPath, const QString& savePath) {
    stopEmulator();
    m_statusMessage.clear();

    m_savePath = savePath;
    if (m_savePath.isEmpty()) {
        QFileInfo info(romPath);
        m_savePath = info.absolutePath() + "/" + info.completeBaseName() + ".sav";
    }

    m_engine = std::make_unique<Pocket::Emulator::MgbaEngine>(m_coreLibraryPath.toStdString());
    if (!m_engine->hasCore()) {
        m_statusMessage = QString::fromStdString(m_engine->coreError());
        update();
        return false;
    }

    if (!m_engine->loadRom(romPath.toStdString())) {
        m_statusMessage = "Failed to load ROM";
        update();
        return false;
    }

    m_audioSink.open(static_cast<int>(m_engine->sampleRate()));

    // Only after loadRom: the core exposes no save RAM until a game is loaded.
    Pocket::Emulator::PersistentGameSave save;
    if (save.loadFromFile(m_savePath.toStdString())) {
        m_engine->loadPersistentSave(save);
    }

    // Set video frame callback
    m_engine->setVideoFrameCallback([this](const uint8_t* pixels, int width, int height, size_t pitch) {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        QImage frame(pixels, width, height, static_cast<qsizetype>(pitch), QImage::Format_RGB32);
        m_currentFrame = frame.copy();
        QMetaObject::invokeMethod(this, "onFrameReady", Qt::QueuedConnection);
    });

    // Set audio sample callback
    m_engine->setAudioSampleCallback(
        [this](const int16_t* samples, size_t frames) { m_audioSink.submit(samples, frames); });

    m_engine->start();
    setHintsVisible(true);
    m_hintTimer.start(4000);
    return true;
}

void EmulatorWidget::setCoreLibraryPath(const QString& path) {
    m_coreLibraryPath = path;
}

void EmulatorWidget::setStatusMessage(const QString& message) {
    stopEmulator();
    m_statusMessage = message;
    update();
}

void EmulatorWidget::stopEmulator() {
    if (m_engine) {
        // Save persistent cartridge save before stopping
        if (!m_savePath.isEmpty()) {
            Pocket::Emulator::PersistentGameSave save = m_engine->getPersistentSave();
            if (!save.isEmpty()) {
                save.saveToFile(m_savePath.toStdString());
            }
        }

        m_engine->stop();
        m_engine.reset(); // Destroys MgbaEngine and unloads core DLL cleanly
    }
    m_audioSink.close();

    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_currentFrame = QImage();
    if (m_statusMessage.isEmpty())
        m_statusMessage = "No ROM loaded";
    update();
}

void EmulatorWidget::onFrameReady() {
    update(); // Redraw GBA canvas on frame ready
}

void EmulatorWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (!m_currentFrame.isNull()) {
            painter.drawImage(rect(), m_currentFrame);
        } else {
            painter.fillRect(rect(), Qt::black);
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, m_statusMessage);
        }
    }

    if (m_hintsVisible) {
        const QSize available(qMax(1, width() * 45 / 100), qMax(1, height() * 40 / 100));
        const QSize overlaySize = m_hintOverlay.preferredSize(available);
        if (!overlaySize.isEmpty()) {
            const QRect overlayBounds(width() - overlaySize.width(), height() - overlaySize.height(),
                                     overlaySize.width(), overlaySize.height());
            m_hintOverlay.paint(painter, overlayBounds);
        }
    }
}

void EmulatorWidget::setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping) {
    m_mapping = std::move(mapping);
    m_hintOverlay.setMapping(m_mapping);
    refreshKeyBindings();
}

void EmulatorWidget::setControllerSystem(const QString& system) {
    if (m_controllerSystem == system)
        return;
    m_controllerSystem = system;
    m_hintOverlay.setSystem(m_controllerSystem);
    refreshKeyBindings();
}

void EmulatorWidget::setHintsVisible(bool visible) {
    if (m_hintsVisible == visible)
        return;
    m_hintsVisible = visible;
    update();
}

void EmulatorWidget::toggleHints() {
    setHintsVisible(!m_hintsVisible);
}

void EmulatorWidget::refreshKeyBindings() {
    // Flattened once per mapping change so a keypress is a hash lookup, not a scan.
    m_keyBindings.clear();
    if (!m_mapping)
        return;

    const auto layout = Pocket::Input::ControllerLayout::forSystem(m_controllerSystem);
    if (!layout)
        return;

    for (const auto& control : layout->controls()) {
        const auto button = Pocket::Input::ControllerMapping::emulatorButtonFor(control.id);
        if (!button)
            continue; // touchscreen, microphone, lid
        const auto binding = m_mapping->binding(m_controllerSystem, control.id);
        if (binding && binding->device == Pocket::Input::InputDevice::Keyboard) {
            m_keyBindings.insert(binding->code, *button);
        }
    }
}

std::optional<Pocket::Emulator::EmulatorButton> EmulatorWidget::buttonForKey(int key) const {
    const auto it = m_keyBindings.constFind(key);
    if (it == m_keyBindings.constEnd())
        return std::nullopt;
    return it.value();
}

void EmulatorWidget::keyPressEvent(QKeyEvent* event) {
    // F1 is reserved for the overlay even if a future mapping assigns it to a game button.
    if (event->key() == Qt::Key_F1) {
        if (!event->isAutoRepeat())
            toggleHints();
        event->accept();
        return;
    }
    // An unbound key must do nothing: this used to fall through to Up.
    if (m_engine && !event->isAutoRepeat()) {
        if (const auto btn = buttonForKey(event->key())) {
            m_engine->sendButtonEvent(*btn, true);
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F1) {
        event->accept();
        return;
    }
    if (m_engine && !event->isAutoRepeat()) {
        if (const auto btn = buttonForKey(event->key())) {
            m_engine->sendButtonEvent(*btn, false);
            event->accept();
            return;
        }
    }
    QWidget::keyReleaseEvent(event);
}

} // namespace Pocket::App
