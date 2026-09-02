#pragma once
#include "pocket/input/ControllerControl.hpp"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <optional>
#include <vector>
namespace Pocket::Input {

class ControllerLayout {
public:
    // Devuelven nullopt y rellenan *error si el JSON es inválido:
    // no parsea, falta "system", falta "controls", un control sin "id",
    // o coordenadas fuera de 0..1.
    static std::optional<ControllerLayout> fromJson(const QByteArray& json, QString* error = nullptr);
    static std::optional<ControllerLayout> fromFile(const QString& path, QString* error = nullptr);

    // Carga assets/controllers/<system minúsculas>/layout.json buscando primero
    // junto al ejecutable y luego en la ruta de assets del repo.
    static std::optional<ControllerLayout> forSystem(const QString& system, QString* error = nullptr);

    QString system() const;          // "GB" | "GBC" | "GBA" | "NDS"
    QString artworkFile() const;     // ruta ABSOLUTA al .svg resuelto
    const std::vector<ControllerControl>& controls() const;

    // Hit test: el último control cuyo rect contiene el punto (los de encima ganan).
    const ControllerControl* controlAt(double nx, double ny) const;
    const ControllerControl* controlById(const QString& id) const;

    static QStringList searchPaths();   // dónde busca assets/controllers

private:
    QString m_system;
    QString m_artwork;
    std::vector<ControllerControl> m_controls;
};

} // namespace Pocket::Input
