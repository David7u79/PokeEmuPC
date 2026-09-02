#include "pocket/input/ControllerLayout.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace Pocket::Input {
namespace {
void setError(QString* error, const QString& message) { if (error) *error = message; }
bool coordinate(const QJsonObject& object, const char* name, double* value)
{
    const QJsonValue json = object.value(QLatin1String(name));
    if (!json.isDouble()) return false;
    *value = json.toDouble();
    return *value >= 0.0 && *value <= 1.0;
}
}

std::optional<ControllerLayout> ControllerLayout::fromJson(const QByteArray& json, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) { setError(error, QStringLiteral("Invalid layout JSON")); return std::nullopt; }
    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("system")).isString() || root.value(QStringLiteral("system")).toString().isEmpty()) { setError(error, QStringLiteral("Missing system")); return std::nullopt; }
    if (!root.value(QStringLiteral("controls")).isArray()) { setError(error, QStringLiteral("Missing controls")); return std::nullopt; }
    ControllerLayout result;
    result.m_system = root.value(QStringLiteral("system")).toString();
    result.m_artwork = root.value(QStringLiteral("artwork")).toString();
    const QJsonArray array = root.value(QStringLiteral("controls")).toArray();
    for (const QJsonValue& value : array) {
        if (!value.isObject()) { setError(error, QStringLiteral("Invalid control")); return std::nullopt; }
        const QJsonObject object = value.toObject();
        if (!object.value(QStringLiteral("id")).isString() || object.value(QStringLiteral("id")).toString().isEmpty()) { setError(error, QStringLiteral("Control missing id")); return std::nullopt; }
        ControllerControl control;
        control.id = object.value(QStringLiteral("id")).toString();
        control.kind = controlKindFromString(object.value(QStringLiteral("kind")).toString());
        if (!coordinate(object, "x", &control.x) || !coordinate(object, "y", &control.y) || !coordinate(object, "width", &control.width) || !coordinate(object, "height", &control.height) || control.x + control.width > 1.0 || control.y + control.height > 1.0) { setError(error, QStringLiteral("Control coordinates must be within 0..1")); return std::nullopt; }
        result.m_controls.push_back(control);
    }
    if (error) error->clear();
    return result;
}

std::optional<ControllerLayout> ControllerLayout::fromFile(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { setError(error, QStringLiteral("Cannot open layout file")); return std::nullopt; }
    auto layout = fromJson(file.readAll(), error);
    if (layout) {
        const QFileInfo info(path);
        layout->m_artwork = QFileInfo(info.dir(), layout->m_artwork).absoluteFilePath();
    }
    return layout;
}

std::optional<ControllerLayout> ControllerLayout::forSystem(const QString& system, QString* error)
{
    const QString directory = system.toLower();
    for (const QString& base : searchPaths()) {
        const QString file = QDir(base).filePath(directory + QStringLiteral("/layout.json"));
        if (QFileInfo(file).isFile()) return fromFile(file, error);
    }
    setError(error, QStringLiteral("No layout found for %1").arg(system));
    return std::nullopt;
}

QString ControllerLayout::system() const { return m_system; }
QString ControllerLayout::artworkFile() const { return m_artwork; }
const std::vector<ControllerControl>& ControllerLayout::controls() const { return m_controls; }
const ControllerControl* ControllerLayout::controlAt(double nx, double ny) const { for (auto it = m_controls.rbegin(); it != m_controls.rend(); ++it) if (it->contains(nx, ny)) return &*it; return nullptr; }
const ControllerControl* ControllerLayout::controlById(const QString& id) const { for (const auto& control : m_controls) if (control.id == id) return &control; return nullptr; }

QStringList ControllerLayout::searchPaths()
{
    QStringList paths;
    const QDir executable(QCoreApplication::applicationDirPath());
    const QString besideExecutable = executable.filePath(QStringLiteral("assets/controllers"));
    if (QFileInfo(besideExecutable).isDir()) paths << QDir(besideExecutable).absolutePath();
    QDir current = executable;
    while (true) {
        const QString candidate = current.filePath(QStringLiteral("assets/controllers"));
        if (QFileInfo(candidate).isDir() && !paths.contains(QDir(candidate).absolutePath())) paths << QDir(candidate).absolutePath();
        if (!current.cdUp()) break;
    }
    return paths;
}

} // namespace Pocket::Input
