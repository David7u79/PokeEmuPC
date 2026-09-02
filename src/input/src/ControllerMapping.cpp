#include "pocket/input/ControllerMapping.hpp"

#include <Qt>

namespace Pocket::Input {
namespace {
QString scopeKey(MappingScope scope, const QString& system) { return scope == MappingScope::Global ? QStringLiteral("GLOBAL") : system; }
const QStringList& presetIds()
{
    static const QStringList ids{QStringLiteral("DPAD_UP"), QStringLiteral("DPAD_DOWN"), QStringLiteral("DPAD_LEFT"), QStringLiteral("DPAD_RIGHT"), QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("L"), QStringLiteral("R"), QStringLiteral("START"), QStringLiteral("SELECT")};
    return ids;
}
}

void ControllerMapping::bind(const QString& system, const QString& controlId, const InputBinding& binding) { m_bindings[scopeKey(m_scope, system)][controlId] = binding; }
void ControllerMapping::clear(const QString& system, const QString& controlId) { m_bindings[scopeKey(m_scope, system)].erase(controlId); }
void ControllerMapping::clearAll(const QString& system) { m_bindings.erase(scopeKey(m_scope, system)); }

std::optional<InputBinding> ControllerMapping::binding(const QString& system, const QString& controlId) const
{
    const auto scopeIt = m_bindings.find(scopeKey(m_scope, system));
    if (scopeIt == m_bindings.end()) return std::nullopt;
    const auto controlIt = scopeIt->second.find(controlId);
    if (controlIt == scopeIt->second.end() || !controlIt->second.isValid()) return std::nullopt;
    return controlIt->second;
}

QStringList ControllerMapping::conflicts(const QString& system, const QString& controlId, const InputBinding& binding) const
{
    QStringList result;
    if (!binding.isValid()) return result;
    const auto scopeIt = m_bindings.find(scopeKey(m_scope, system));
    if (scopeIt == m_bindings.end()) return result;
    for (const auto& [id, candidate] : scopeIt->second) if (id != controlId && candidate == binding) result << id;
    return result;
}

void ControllerMapping::resetToDefaults(const QString& system)
{
    const auto defaults = keyboardPreset();
    const QString key = scopeKey(m_scope, system);
    m_bindings[key] = defaults.m_bindings.at(QStringLiteral("GLOBAL"));
}

ControllerMapping ControllerMapping::keyboardPreset()
{
    ControllerMapping mapping;
    const int keys[] = {Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D, Qt::Key_L, Qt::Key_K, Qt::Key_I, Qt::Key_J, Qt::Key_Q, Qt::Key_E, Qt::Key_Return, Qt::Key_Space};
    for (int index = 0; index < presetIds().size(); ++index) mapping.bind(QString(), presetIds()[index], {InputDevice::Keyboard, keys[index]});
    return mapping;
}

ControllerMapping ControllerMapping::genericGamepadPreset()
{
    ControllerMapping mapping;
    for (int index = 0; index < presetIds().size(); ++index) mapping.bind(QString(), presetIds()[index], {InputDevice::Gamepad, index});
    return mapping;
}

MappingScope ControllerMapping::scope() const { return m_scope; }
void ControllerMapping::setScope(MappingScope scope) { m_scope = scope; }

void ControllerMapping::save(QSettings& settings) const
{
    settings.beginGroup(QStringLiteral("controllerMapping"));
    settings.remove(QString());
    settings.setValue(QStringLiteral("scope"), m_scope == MappingScope::Global ? QStringLiteral("Global") : QStringLiteral("PerSystem"));
    for (const auto& [scopeName, controls] : m_bindings) {
        settings.beginGroup(scopeName);
        for (const auto& [id, value] : controls) settings.setValue(id, value.serialize());
        settings.endGroup();
    }
    settings.endGroup();
}

bool ControllerMapping::load(QSettings& settings)
{
    settings.beginGroup(QStringLiteral("controllerMapping"));
    if (!settings.contains(QStringLiteral("scope"))) { settings.endGroup(); return false; }
    m_scope = settings.value(QStringLiteral("scope")).toString() == QStringLiteral("PerSystem") ? MappingScope::PerSystem : MappingScope::Global;
    m_bindings.clear();
    const QStringList groups = settings.childGroups();
    for (const QString& scopeName : groups) {
        settings.beginGroup(scopeName);
        for (const QString& id : settings.childKeys()) {
            const InputBinding value = InputBinding::deserialize(settings.value(id).toString());
            if (value.isValid()) m_bindings[scopeName][id] = value;
        }
        settings.endGroup();
    }
    settings.endGroup();
    return true;
}

std::optional<Pocket::Emulator::EmulatorButton> ControllerMapping::emulatorButtonFor(const QString& controlId)
{
    using Button = Pocket::Emulator::EmulatorButton;
    if (controlId == QStringLiteral("DPAD_UP")) return Button::Up;
    if (controlId == QStringLiteral("DPAD_DOWN")) return Button::Down;
    if (controlId == QStringLiteral("DPAD_LEFT")) return Button::Left;
    if (controlId == QStringLiteral("DPAD_RIGHT")) return Button::Right;
    if (controlId == QStringLiteral("A")) return Button::A;
    if (controlId == QStringLiteral("B")) return Button::B;
    if (controlId == QStringLiteral("X")) return Button::X;
    if (controlId == QStringLiteral("Y")) return Button::Y;
    if (controlId == QStringLiteral("L")) return Button::L;
    if (controlId == QStringLiteral("R")) return Button::R;
    if (controlId == QStringLiteral("START")) return Button::Start;
    if (controlId == QStringLiteral("SELECT")) return Button::Select;
    return std::nullopt;
}

} // namespace Pocket::Input
