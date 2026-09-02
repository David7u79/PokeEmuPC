#pragma once
#include <QString>
namespace Pocket::Input {

enum class InputDevice { None, Keyboard, Gamepad };

struct InputBinding {
    InputDevice device{InputDevice::None};
    int code{0};   // Qt::Key para Keyboard; índice de botón para Gamepad

    bool isValid() const;
    QString label() const;              // "Keyboard X", "Gamepad Button 3", "" si !isValid
    QString serialize() const;          // "keyboard:88" / "gamepad:3" / ""
    static InputBinding deserialize(const QString& s);   // inválido si no parsea
    bool operator==(const InputBinding& o) const;
    bool operator!=(const InputBinding& o) const;
};

} // namespace Pocket::Input
