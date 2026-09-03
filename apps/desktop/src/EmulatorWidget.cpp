#include "EmulatorWidget.hpp"
#include "pocket/input/ControllerLayout.hpp"
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <cstring>

namespace Pocket::App {

EmulatorWidget::EmulatorWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(480, 320); // 2x scale GBA 240x160 resolution
    m_hintOverlay.setSystem(m_controllerSystem);
    QSettings settings("PocketPartnerProject", "PocketPartner");
    m_hintsVisible = settings.value("emulator/showControlHints", true).toBool();
    m_viewMode = settings.value("emulator/viewMode", 0).toInt() == 1 ? EmulatorViewMode::FullScreen
                                                                      : EmulatorViewMode::ConsoleFrame;
    m_captureBlinkTimer.setInterval(350);
    connect(&m_captureBlinkTimer, &QTimer::timeout, this, [this] {
        m_captureBlinkOn = !m_captureBlinkOn;
        m_hintOverlay.setCaptureHighlight(m_capturingControlId, m_captureBlinkOn);
        update();
    });
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
    releaseMouseControl();
    m_hintOverlay.clearPressed();
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

    painter.fillRect(rect(), Qt::black);
    const bool framed = m_viewMode == EmulatorViewMode::ConsoleFrame && m_hintOverlay.isValid();
    const QRectF screen = framed ? m_hintOverlay.controlRect(QStringLiteral("SCREEN"), size()) : QRectF(rect());
    if (framed)
        m_hintOverlay.paintFrame(painter, size());
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (!m_currentFrame.isNull()) {
            painter.drawImage(screen, m_currentFrame, m_currentFrame.rect());
        } else {
            painter.setPen(Qt::white);
            painter.drawText(screen, Qt::AlignCenter, m_statusMessage);
        }
    }
    if (framed)
        m_hintOverlay.paintPressed(painter, size());
    if (framed && m_hintsVisible)
        m_hintOverlay.paintKeyLabels(painter, size());
}

void EmulatorWidget::setViewMode(EmulatorViewMode mode) {
    if (m_viewMode == mode)
        return;
    m_viewMode = mode;
    QSettings("PocketPartnerProject", "PocketPartner").setValue("emulator/viewMode", mode == EmulatorViewMode::FullScreen ? 1 : 0);
    update();
}

void EmulatorWidget::toggleViewMode() { setViewMode(m_viewMode == EmulatorViewMode::ConsoleFrame ? EmulatorViewMode::FullScreen : EmulatorViewMode::ConsoleFrame); }

void EmulatorWidget::setControllerMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping) {
    m_mapping = std::move(mapping);
    m_hintOverlay.setMapping(m_mapping);
    refreshKeyBindings();
}

void EmulatorWidget::setControllerSystem(const QString& system) {
    if (m_controllerSystem == system)
        return;
    m_controllerSystem = system;
    releaseMouseControl();
    m_hintOverlay.clearPressed();
    m_hintOverlay.setCaptureHighlight(m_capturingControlId, false);
    m_capturingControlId.clear();
    m_captureBlinkTimer.stop();
    m_hintOverlay.setSystem(m_controllerSystem);
    refreshKeyBindings();
}

void EmulatorWidget::setHintsVisible(bool visible) {
    if (m_hintsVisible == visible)
        return;
    m_hintsVisible = visible;
    // Remembered, so the choice survives closing the app.
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/showControlHints", visible);
    update();
}

void EmulatorWidget::toggleHints() {
    setHintsVisible(!m_hintsVisible);
}

void EmulatorWidget::refreshKeyBindings() {
    // Flattened once per mapping change so a keypress is a hash lookup, not a scan.
    m_keyBindings.clear();
    m_keyControlIds.clear();
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
            m_keyControlIds.insert(binding->code, control.id);
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
    if (event->key() == Qt::Key_F2) {
        if (!event->isAutoRepeat())
            toggleViewMode();
        event->accept();
        return;
    }
    if (!m_capturingControlId.isEmpty()) {
        if (event->key() == Qt::Key_Escape) {
            m_hintOverlay.setPressed(m_capturingControlId, false);
            m_hintOverlay.setCaptureHighlight(m_capturingControlId, false);
            m_capturingControlId.clear();
            m_captureBlinkTimer.stop();
            update();
        } else if (!event->isAutoRepeat()) {
            applyCapturedBinding(event->key());
        }
        event->accept();
        return;
    }
    // An unbound key must do nothing: this used to fall through to Up.
    if (m_engine && !event->isAutoRepeat()) {
        if (const auto btn = buttonForKey(event->key())) {
            m_engine->sendButtonEvent(*btn, true);
            m_hintOverlay.setPressed(m_keyControlIds.value(event->key()), true);
            update();
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
    if (event->key() == Qt::Key_F2) {
        event->accept();
        return;
    }
    if (!m_capturingControlId.isEmpty()) {
        event->accept();
        return;
    }
    if (m_engine && !event->isAutoRepeat()) {
        if (const auto btn = buttonForKey(event->key())) {
            m_engine->sendButtonEvent(*btn, false);
            m_hintOverlay.setPressed(m_keyControlIds.value(event->key()), false);
            update();
            event->accept();
            return;
        }
    }
    QWidget::keyReleaseEvent(event);
}

void EmulatorWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_viewMode == EmulatorViewMode::FullScreen || !m_hintOverlay.isValid()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QRectF artwork = m_hintOverlay.artworkRect(size());
    const auto layout = Pocket::Input::ControllerLayout::forSystem(m_controllerSystem);
    if (!layout || !artwork.contains(event->position())) {
        QWidget::mousePressEvent(event);
        return;
    }
    const double x = (event->position().x() - artwork.left()) / artwork.width();
    const double y = (event->position().y() - artwork.top()) / artwork.height();
    const auto* control = layout->controlAt(x, y);
    if (!control || !control->isBindable()) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        const QString name = controlDisplayName(control->id);
        QAction* reassign = menu.addAction(QStringLiteral("Reasignar %1").arg(name));
        QAction* clear = nullptr;
        if (m_mapping && m_mapping->binding(m_controllerSystem, control->id))
            clear = menu.addAction(QStringLiteral("Quitar asignación"));
        QAction* selected = menu.exec(event->globalPosition().toPoint());
        if (selected == reassign
            && QMessageBox::question(this, QStringLiteral("Reasignar %1").arg(name),
                                      QStringLiteral("¿Asignar una tecla nueva a %1?").arg(name)) == QMessageBox::Yes)
            beginCapture(control->id);
        else if (selected == clear && m_mapping) {
            m_mapping->clear(m_controllerSystem, control->id);
            saveMapping(); refreshKeyBindings(); emit mappingEdited(); update();
        }
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_engine) {
        if (const auto button = Pocket::Input::ControllerMapping::emulatorButtonFor(control->id)) {
            releaseMouseControl();
            m_mousePressedControlId = control->id;
            m_engine->sendButtonEvent(*button, true);
            m_hintOverlay.setPressed(control->id, true);
            update(); event->accept(); return;
        }
    }
    QWidget::mousePressEvent(event);
}

void EmulatorWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        releaseMouseControl();
    QWidget::mouseReleaseEvent(event);
}

void EmulatorWidget::leaveEvent(QEvent* event)
{
    releaseMouseControl();
    QWidget::leaveEvent(event);
}

void EmulatorWidget::releaseMouseControl()
{
    if (m_mousePressedControlId.isEmpty()) return;
    if (m_engine) {
        if (const auto button = Pocket::Input::ControllerMapping::emulatorButtonFor(m_mousePressedControlId))
            m_engine->sendButtonEvent(*button, false);
    }
    m_hintOverlay.setPressed(m_mousePressedControlId, false);
    m_mousePressedControlId.clear();
    update();
}

void EmulatorWidget::saveMapping()
{
    if (!m_mapping) return;
    QSettings settings("PocketPartnerProject", "PocketPartner");
    m_mapping->save(settings);
}

void EmulatorWidget::beginCapture(const QString& controlId)
{
    if (!m_mapping) return;
    m_capturingControlId = controlId;
    m_captureBlinkOn = true;
    m_hintOverlay.setPressed(controlId, true);
    m_hintOverlay.setCaptureHighlight(controlId, true);
    m_captureBlinkTimer.start();
    setFocus(); update();
}

void EmulatorWidget::applyCapturedBinding(int key)
{
    if (!m_mapping || m_capturingControlId.isEmpty()) return;
    const QString controlId = m_capturingControlId;
    const Pocket::Input::InputBinding binding{Pocket::Input::InputDevice::Keyboard, key};
    const QStringList conflicts = m_mapping->conflicts(m_controllerSystem, controlId, binding);
    if (!conflicts.isEmpty()) {
        const QString other = controlDisplayName(conflicts.first());
        if (QMessageBox::question(this, QStringLiteral("Reasignar %1").arg(controlDisplayName(controlId)),
                                  QStringLiteral("La tecla ya está asignada a %1. ¿Reemplazarla?").arg(other)) != QMessageBox::Yes) {
            m_hintOverlay.setPressed(controlId, false);
            m_hintOverlay.setCaptureHighlight(controlId, false);
            m_capturingControlId.clear(); m_captureBlinkTimer.stop(); update(); return;
        }
        for (const QString& conflict : conflicts) m_mapping->clear(m_controllerSystem, conflict);
    }
    m_mapping->bind(m_controllerSystem, controlId, binding);
    saveMapping(); refreshKeyBindings(); emit mappingEdited();
    m_hintOverlay.setPressed(controlId, false);
    m_hintOverlay.setCaptureHighlight(controlId, false);
    m_capturingControlId.clear(); m_captureBlinkTimer.stop(); update();
}

} // namespace Pocket::App
