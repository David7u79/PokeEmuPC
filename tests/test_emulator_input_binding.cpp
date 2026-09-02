#include <QtTest/QtTest>
#include <memory>
#include "EmulatorWidget.hpp"
#include "pocket/input/ControllerMapping.hpp"

using Pocket::App::EmulatorWidget;
using Pocket::Emulator::EmulatorButton;
using Pocket::Input::ControllerMapping;
using Pocket::Input::InputBinding;
using Pocket::Input::InputDevice;
using Pocket::Input::MappingScope;

// Closes the loop the other suites leave open: a binding configured in
// Settings -> Controls has to come back out as the right EmulatorButton.
class TestEmulatorInputBinding : public QObject {
    Q_OBJECT
private slots:
    void keyboardPresetReachesTheEngine() {
        auto mapping = std::make_shared<ControllerMapping>(ControllerMapping::keyboardPreset());
        EmulatorWidget widget;
        widget.setControllerMapping(mapping);
        widget.setControllerSystem("GBA");

        QCOMPARE(widget.buttonForKey(Qt::Key_L), std::optional<EmulatorButton>(EmulatorButton::A));
        QCOMPARE(widget.buttonForKey(Qt::Key_K), std::optional<EmulatorButton>(EmulatorButton::B));
        QCOMPARE(widget.buttonForKey(Qt::Key_W), std::optional<EmulatorButton>(EmulatorButton::Up));
        QCOMPARE(widget.buttonForKey(Qt::Key_Return), std::optional<EmulatorButton>(EmulatorButton::Start));
    }

    void unboundKeyDoesNothing() {
        auto mapping = std::make_shared<ControllerMapping>(ControllerMapping::keyboardPreset());
        EmulatorWidget widget;
        widget.setControllerMapping(mapping);
        widget.setControllerSystem("GBA");

        // This used to fall through to Up, so every stray letter nudged the player.
        QVERIFY(!widget.buttonForKey(Qt::Key_F7).has_value());
        QVERIFY(!widget.buttonForKey(Qt::Key_Escape).has_value());
    }

    void rebindingTakesEffectAfterRefresh() {
        auto mapping = std::make_shared<ControllerMapping>(ControllerMapping::keyboardPreset());
        EmulatorWidget widget;
        widget.setControllerMapping(mapping);
        widget.setControllerSystem("GBA");
        QVERIFY(!widget.buttonForKey(Qt::Key_M).has_value());

        mapping->clear("GBA", "A");
        mapping->bind("GBA", "A", InputBinding{InputDevice::Keyboard, Qt::Key_M});
        widget.refreshKeyBindings();

        QCOMPARE(widget.buttonForKey(Qt::Key_M), std::optional<EmulatorButton>(EmulatorButton::A));
        QVERIFY(!widget.buttonForKey(Qt::Key_L).has_value());
    }

    void perSystemMappingsDiffer() {
        auto mapping = std::make_shared<ControllerMapping>(ControllerMapping::keyboardPreset());
        mapping->setScope(MappingScope::PerSystem);
        mapping->bind("GBA", "A", InputBinding{InputDevice::Keyboard, Qt::Key_1});
        mapping->bind("NDS", "A", InputBinding{InputDevice::Keyboard, Qt::Key_2});

        EmulatorWidget widget;
        widget.setControllerMapping(mapping);

        widget.setControllerSystem("GBA");
        QCOMPARE(widget.buttonForKey(Qt::Key_1), std::optional<EmulatorButton>(EmulatorButton::A));
        QVERIFY(!widget.buttonForKey(Qt::Key_2).has_value());

        widget.setControllerSystem("NDS");
        QCOMPARE(widget.buttonForKey(Qt::Key_2), std::optional<EmulatorButton>(EmulatorButton::A));
        QVERIFY(!widget.buttonForKey(Qt::Key_1).has_value());
    }

    void withoutAMappingNothingIsBound() {
        EmulatorWidget widget;
        QVERIFY(!widget.buttonForKey(Qt::Key_L).has_value());
    }
};

QTEST_MAIN(TestEmulatorInputBinding)
#include "test_emulator_input_binding.moc"
