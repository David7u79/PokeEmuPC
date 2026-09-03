#include "AudioSink.hpp"
#include "SaveStateSlots.hpp"
#include "pocket/emulator/LibretroEngineBase.hpp"

#include <QTest>

class EmulatorControlsTest : public QObject {
    Q_OBJECT
private slots:
    void speedMultiplierClamps() {
        Pocket::Emulator::LibretroEngineBase engine("");
        engine.setSpeedMultiplier(0); QCOMPARE(engine.speedMultiplier(), 1);
        engine.setSpeedMultiplier(9); QCOMPARE(engine.speedMultiplier(), 5);
        engine.setSpeedMultiplier(3); QCOMPARE(engine.speedMultiplier(), 3);
    }
    void volumeAppliesAndClamps() {
        int16_t samples[] = {1000, -1000, 32767, -32768};
        Pocket::App::applyVolume(samples, 4, 1.0f); QCOMPARE(samples[0], int16_t(1000)); QCOMPARE(samples[3], int16_t(-32768));
        Pocket::App::applyVolume(samples, 4, 0.0f); QCOMPARE(samples[0], int16_t(0)); QCOMPARE(samples[3], int16_t(0));
        int16_t half[] = {2000, -2000, 32767, -32768};
        Pocket::App::applyVolume(half, 4, 0.5f); QCOMPARE(half[0], int16_t(1000)); QCOMPARE(half[1], int16_t(-1000)); QCOMPARE(half[2], int16_t(16383)); QCOMPARE(half[3], int16_t(-16384));
    }
    void saveStateSlots() {
        for (int slot = 1; slot <= 5; ++slot)
            QCOMPARE(Pocket::App::saveStatePath("C:/games/test.sav", slot), QString("C:/games/test.state%1").arg(slot));
        QCOMPARE(Pocket::App::saveStatePath("C:/games/test.sav", 0), QString("C:/games/test.stateauto"));
        QVERIFY(Pocket::App::saveStatePath({}, 1).isEmpty());
    }
    void noCoreHasNoSaveStates() {
        Pocket::Emulator::LibretroEngineBase engine("");
        QVERIFY(!engine.supportsSaveStates());
    }
};
QTEST_MAIN(EmulatorControlsTest)
#include "test_emulator_controls.moc"
