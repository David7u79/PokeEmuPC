#include "pocket/input/InputBinding.hpp"
#include <QStringList>

namespace Pocket::Input {

bool InputBinding::isValid() const
{
    return device == InputDevice::Keyboard || (device == InputDevice::Gamepad && code >= 0);
}

QString InputBinding::label() const
{
    if (!isValid()) return {};
    if (device == InputDevice::Gamepad) return QStringLiteral("Gamepad Button %1").arg(code);
    if (code == 0x01000004) return QStringLiteral("Keyboard Return");
    if (code == 0x20) return QStringLiteral("Keyboard Space");
    return QStringLiteral("Keyboard %1").arg(QChar(code));
}

QString InputBinding::serialize() const
{
    if (!isValid()) return {};
    return QStringLiteral("%1:%2").arg(device == InputDevice::Keyboard ? QStringLiteral("keyboard") : QStringLiteral("gamepad")).arg(code);
}

InputBinding InputBinding::deserialize(const QString& s)
{
    const QStringList parts = s.split(QLatin1Char(':'));
    bool ok = false;
    if (parts.size() != 2) return {};
    const int code = parts[1].toInt(&ok);
    if (!ok) return {};
    if (parts[0] == QStringLiteral("keyboard")) return {InputDevice::Keyboard, code};
    if (parts[0] == QStringLiteral("gamepad") && code >= 0) return {InputDevice::Gamepad, code};
    return {};
}

bool InputBinding::operator==(const InputBinding& o) const { return device == o.device && code == o.code; }
bool InputBinding::operator!=(const InputBinding& o) const { return !(*this == o); }

} // namespace Pocket::Input
