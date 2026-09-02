#include "EmulatorWidget.hpp"
#include <QPainter>
#include <QDebug>
#include <QFileInfo>

namespace Pocket::App {

EmulatorWidget::EmulatorWidget(QWidget *parent)
    : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(480, 320); // 2x scale GBA 240x160 resolution
    initAudio();
}

EmulatorWidget::~EmulatorWidget() {
    stopEmulator();
    closeAudio();
}

void EmulatorWidget::initAudio() {
#ifdef _WIN32
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&m_waveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
        m_audioInitialized = true;
        for (int i = 0; i < 4; ++i) {
            m_audioBuffers[i].resize(4096, 0);
            std::memset(&m_waveHeaders[i], 0, sizeof(WAVEHDR));
            m_waveHeaders[i].lpData = reinterpret_cast<LPSTR>(m_audioBuffers[i].data());
            m_waveHeaders[i].dwBufferLength = static_cast<DWORD>(m_audioBuffers[i].size() * sizeof(int16_t));
            waveOutPrepareHeader(m_waveOut, &m_waveHeaders[i], sizeof(WAVEHDR));
        }
    }
#endif
}

void EmulatorWidget::writeAudioSamples(const int16_t* samples, size_t frames) {
#ifdef _WIN32
    if (!m_audioInitialized || !m_waveOut || !samples || frames == 0) return;

    WAVEHDR& hdr = m_waveHeaders[m_currentBufferIndex];
    if (hdr.dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(m_waveOut, &hdr, sizeof(WAVEHDR));
    }

    size_t sampleCount = frames * 2; // stereo
    m_audioBuffers[m_currentBufferIndex].assign(samples, samples + sampleCount);

    hdr.lpData = reinterpret_cast<LPSTR>(m_audioBuffers[m_currentBufferIndex].data());
    hdr.dwBufferLength = static_cast<DWORD>(sampleCount * sizeof(int16_t));
    hdr.dwFlags = 0;

    waveOutPrepareHeader(m_waveOut, &hdr, sizeof(WAVEHDR));
    waveOutWrite(m_waveOut, &hdr, sizeof(WAVEHDR));

    m_currentBufferIndex = (m_currentBufferIndex + 1) % 4;
#endif
}

void EmulatorWidget::closeAudio() {
#ifdef _WIN32
    if (m_waveOut) {
        waveOutReset(m_waveOut);
        for (int i = 0; i < 4; ++i) {
            if (m_waveHeaders[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(m_waveOut, &m_waveHeaders[i], sizeof(WAVEHDR));
            }
        }
        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }
    m_audioInitialized = false;
#endif
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
    m_engine->setAudioSampleCallback([this](const int16_t* samples, size_t frames) {
        writeAudioSamples(samples, frames);
    });

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

    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_currentFrame = QImage();
    if (m_statusMessage.isEmpty()) m_statusMessage = "No ROM loaded";
    update();
}

void EmulatorWidget::onFrameReady() {
    update(); // Redraw GBA canvas on frame ready
}

void EmulatorWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (!m_currentFrame.isNull()) {
        painter.drawImage(rect(), m_currentFrame);
    } else {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, m_statusMessage);
    }
}

Pocket::Emulator::EmulatorButton EmulatorWidget::mapKeyToButton(int key) const {
    switch (key) {
        case Qt::Key_Up:
        case Qt::Key_W:      return Pocket::Emulator::EmulatorButton::Up;
        case Qt::Key_Down:
        case Qt::Key_S:      return Pocket::Emulator::EmulatorButton::Down;
        case Qt::Key_Left:
        case Qt::Key_A:      return Pocket::Emulator::EmulatorButton::Left;
        case Qt::Key_Right:
        case Qt::Key_D:      return Pocket::Emulator::EmulatorButton::Right;
        case Qt::Key_Z:
        case Qt::Key_J:      return Pocket::Emulator::EmulatorButton::A;
        case Qt::Key_X:
        case Qt::Key_K:      return Pocket::Emulator::EmulatorButton::B;
        case Qt::Key_U:      return Pocket::Emulator::EmulatorButton::L;
        case Qt::Key_I:      return Pocket::Emulator::EmulatorButton::R;
        case Qt::Key_Return: return Pocket::Emulator::EmulatorButton::Start;
        case Qt::Key_Shift:
        case Qt::Key_Space:  return Pocket::Emulator::EmulatorButton::Select;
        default:             return Pocket::Emulator::EmulatorButton::Up;
    }
}

void EmulatorWidget::keyPressEvent(QKeyEvent *event) {
    if (m_engine && !event->isAutoRepeat()) {
        Pocket::Emulator::EmulatorButton btn = mapKeyToButton(event->key());
        m_engine->sendButtonEvent(btn, true);
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent *event) {
    if (m_engine && !event->isAutoRepeat()) {
        Pocket::Emulator::EmulatorButton btn = mapKeyToButton(event->key());
        m_engine->sendButtonEvent(btn, false);
        event->accept();
    } else {
        QWidget::keyReleaseEvent(event);
    }
}

} // namespace Pocket::App
