#include "pocket/input/ControllerControl.hpp"

namespace Pocket::Input {

bool ControllerControl::contains(double nx, double ny) const
{
    return nx >= x && nx <= x + width && ny >= y && ny <= y + height;
}

bool ControllerControl::isBindable() const
{
    return kind != ControlKind::Touchscreen && kind != ControlKind::Microphone && kind != ControlKind::Lid;
}

QString controlKindToString(ControlKind kind)
{
    switch (kind) {
    case ControlKind::Button: return QStringLiteral("Button");
    case ControlKind::DPad: return QStringLiteral("DPad");
    case ControlKind::Touchscreen: return QStringLiteral("Touchscreen");
    case ControlKind::Microphone: return QStringLiteral("Microphone");
    case ControlKind::Lid: return QStringLiteral("Lid");
    }
    return QStringLiteral("Button");
}

ControlKind controlKindFromString(const QString& s)
{
    if (s == QStringLiteral("DPad")) return ControlKind::DPad;
    if (s == QStringLiteral("Touchscreen")) return ControlKind::Touchscreen;
    if (s == QStringLiteral("Microphone")) return ControlKind::Microphone;
    if (s == QStringLiteral("Lid")) return ControlKind::Lid;
    return ControlKind::Button;
}

} // namespace Pocket::Input
