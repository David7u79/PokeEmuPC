#include <QtTest>
#include <QTemporaryDir>
#include "pocket/input/ControllerMapping.hpp"

using namespace Pocket::Input;
using Button = Pocket::Emulator::EmulatorButton;

class ControllerMappingTest : public QObject {
    Q_OBJECT
private slots:
    void bindingAndConflicts();
    void presetsAndEmulatorMap();
    void persistenceResetScopeAndSerialization();
};

void ControllerMappingTest::bindingAndConflicts()
{
    ControllerMapping mapping; const InputBinding key{InputDevice::Keyboard, Qt::Key_A};
    mapping.bind("GBA", "A", key); QCOMPARE(mapping.binding("NDS", "A").value(), key);
    mapping.bind("GBA", "B", key); QCOMPARE(mapping.conflicts("GBA", "A", key), QStringList{"B"});
    QVERIFY(mapping.conflicts("GBA", "A", {InputDevice::Keyboard, Qt::Key_B}).isEmpty());
    mapping.clear("GBA", "A"); QVERIFY(!mapping.binding("GBA", "A"));
}

void ControllerMappingTest::presetsAndEmulatorMap()
{
    const auto preset = ControllerMapping::keyboardPreset();
    const QList<QPair<QString, Button>> expected{{"DPAD_UP", Button::Up},{"DPAD_DOWN",Button::Down},{"DPAD_LEFT",Button::Left},{"DPAD_RIGHT",Button::Right},{"A",Button::A},{"B",Button::B},{"X",Button::X},{"Y",Button::Y},{"L",Button::L},{"R",Button::R},{"START",Button::Start},{"SELECT",Button::Select}};
    for (const auto& item : expected) { QVERIFY(preset.binding("GBA", item.first)); QCOMPARE(ControllerMapping::emulatorButtonFor(item.first).value(), item.second); }
    for (const QString& id : {QStringLiteral("TOUCHSCREEN"),QStringLiteral("MICROPHONE"),QStringLiteral("LID")}) QVERIFY(!ControllerMapping::emulatorButtonFor(id));
}

void ControllerMappingTest::persistenceResetScopeAndSerialization()
{
    QTemporaryDir directory; QVERIFY(directory.isValid()); const QString path = directory.filePath("mapping.ini");
    ControllerMapping original; original.bind("GBA", "A", {InputDevice::Keyboard, Qt::Key_Z}); original.setScope(MappingScope::PerSystem); original.bind("GBA", "B", {InputDevice::Gamepad, 3});
    { QSettings settings(path, QSettings::IniFormat); original.save(settings); settings.sync(); }
    ControllerMapping loaded; { QSettings settings(path, QSettings::IniFormat); QVERIFY(loaded.load(settings)); }
    QCOMPARE(loaded.scope(), MappingScope::PerSystem); QCOMPARE(loaded.binding("GBA", "B").value(), InputBinding(InputDevice::Gamepad, 3)); QVERIFY(!loaded.binding("NDS", "B"));
    loaded.resetToDefaults("GBA"); QCOMPARE(loaded.binding("GBA", "A").value(), ControllerMapping::keyboardPreset().binding("GBA", "A").value());
    const InputBinding input{InputDevice::Keyboard, Qt::Key_X}; QCOMPARE(InputBinding::deserialize(input.serialize()), input); QVERIFY(!InputBinding::deserialize("nonsense").isValid());
}

QTEST_APPLESS_MAIN(ControllerMappingTest)
#include "test_controller_mapping.moc"
