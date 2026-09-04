#pragma once

#include <QObject>
#include <QTimer>

namespace Pocket::App {

// Polls the first connected XInput pad. The app had gamepad bindings in the
// model but nothing ever read a physical device, so a controller could be
// mapped and still do nothing.
//
// ponytail: XInput only. Any pad Windows exposes as a 360 pad works, which is
// what generic pads do. Move to SDL if a DirectInput-only pad shows up.
class GamepadReader : public QObject {
    Q_OBJECT

public:
    explicit GamepadReader(QObject* parent = nullptr);

    bool isConnected() const { return m_connected; }
    // Off while nobody is listening: the mapper only needs the pad during capture.
    void setActive(bool active);

signals:
    // index is the position in ControllerMapping::presetControlIds().
    void buttonChanged(int index, bool pressed);
    void connectionChanged(bool connected);

private:
    void poll();

    QTimer m_timer;
    int m_pad{-1};
    bool m_connected{false};
    quint32 m_previous{0};
    int m_idleTicks{0};
};

} // namespace Pocket::App
