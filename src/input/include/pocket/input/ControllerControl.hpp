#pragma once
#include <QString>
namespace Pocket::Input {

enum class ControlKind { Button, DPad, Touchscreen, Microphone, Lid, Screen };

struct ControllerControl {
    QString id;                 // "A", "B", "X", "Y", "L", "R", "START", "SELECT",
                                // "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT",
                                // "TOUCHSCREEN", "MICROPHONE", "LID"
    ControlKind kind{ControlKind::Button};
    double x{0.0}, y{0.0}, width{0.0}, height{0.0};   // normalizados 0..1

    bool contains(double nx, double ny) const;   // hit test en espacio normalizado
    bool isBindable() const;   // false para Touchscreen/Microphone/Lid
};

QString controlKindToString(ControlKind kind);
ControlKind controlKindFromString(const QString& s);   // Button si no reconoce

} // namespace Pocket::Input
