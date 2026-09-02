#pragma once
#include "pocket/input/InputBinding.hpp"
#include "pocket/emulator/EmulatorEngine.hpp"
#include <QSettings>
#include <QString>
#include <QStringList>
#include <map>
#include <optional>
namespace Pocket::Input {

enum class MappingScope { Global, PerSystem };

class ControllerMapping {
public:
    void bind(const QString& system, const QString& controlId, const InputBinding& binding);
    void clear(const QString& system, const QString& controlId);
    void clearAll(const QString& system);
    std::optional<InputBinding> binding(const QString& system, const QString& controlId) const;

    // Ids de OTROS controles del mismo ámbito ya ligados a ese input. Vacío = sin conflicto.
    // bind() NO impide el conflicto: es la UI quien decide. Pero conflicts() debe ser exacto.
    QStringList conflicts(const QString& system, const QString& controlId,
                          const InputBinding& binding) const;

    void resetToDefaults(const QString& system);

    static ControllerMapping keyboardPreset();
    static ControllerMapping genericGamepadPreset();

    MappingScope scope() const;
    void setScope(MappingScope scope);   // Global: todos los sistemas comparten una tabla

    void save(QSettings& settings) const;
    bool load(QSettings& settings);

    // Ninguno para TOUCHSCREEN/MICROPHONE/LID.
    static std::optional<Pocket::Emulator::EmulatorButton> emulatorButtonFor(const QString& controlId);

private:
    MappingScope m_scope{MappingScope::Global};
    std::map<QString, std::map<QString, InputBinding>> m_bindings;  // clave de ámbito -> controlId -> binding
};

} // namespace Pocket::Input
