#include <QtTest/QtTest>

#include "DevAssets.hpp"
#include "pocket/emulator/LibretroCoreSession.hpp"
#include "pocket/emulator/LibretroEngineBase.hpp"
#include "pocket/emulator/MelonDsEngine.hpp"
#include "pocket/emulator/MgbaEngine.hpp"

namespace {
class GameInfoProbe : public Pocket::Emulator::LibretroEngineBase {
public:
    using LibretroEngineBase::LibretroEngineBase;
    using LibretroEngineBase::buildGameInfo;
};
} // namespace

class TestLibretroCoreSession : public QObject {
    Q_OBJECT

private slots:
    void ownershipIsExclusiveAndReentrant() {
        auto* a = reinterpret_cast<Pocket::Emulator::LibretroEngineBase*>(static_cast<uintptr_t>(1));
        auto* b = reinterpret_cast<Pocket::Emulator::LibretroEngineBase*>(static_cast<uintptr_t>(2));
        Pocket::Emulator::LibretroCoreSession::release(a);
        Pocket::Emulator::LibretroCoreSession::release(b);

        QVERIFY(Pocket::Emulator::LibretroCoreSession::acquire(a));
        QVERIFY(Pocket::Emulator::LibretroCoreSession::acquire(a));
        QVERIFY(!Pocket::Emulator::LibretroCoreSession::acquire(b));
        QVERIFY(Pocket::Emulator::LibretroCoreSession::isHeldBy(a));

        Pocket::Emulator::LibretroCoreSession::release(b);
        QVERIFY(Pocket::Emulator::LibretroCoreSession::isHeldBy(a));
        Pocket::Emulator::LibretroCoreSession::release(a);
        QVERIFY(Pocket::Emulator::LibretroCoreSession::acquire(b));
        Pocket::Emulator::LibretroCoreSession::release(b);
    }

    void gameInfoRespectsNeedFullpath() {
        const char path[] = "test.rom";
        const uint8_t bytes[] = {1, 2, 3};
        const retro_game_info fullpath = GameInfoProbe::buildGameInfo(path, bytes, sizeof(bytes), true);
        QCOMPARE(fullpath.path, path);
        QVERIFY(fullpath.data == nullptr);
        QCOMPARE(fullpath.size, static_cast<size_t>(0));

        const retro_game_info inMemory = GameInfoProbe::buildGameInfo(path, bytes, sizeof(bytes), false);
        QCOMPARE(inMemory.path, path);
        QCOMPARE(inMemory.data, static_cast<const void*>(bytes));
        QCOMPARE(inMemory.size, sizeof(bytes));
    }

    void realCoresCannotCoexist() {
        const QString mgbaPath = DevAssets::mgbaCore();
        const QString melonPath = DevAssets::melonDsCore();
        if (mgbaPath.isEmpty() || melonPath.isEmpty())
            QSKIP("set or discover both mGBA and melonDS cores");

        {
            Pocket::Emulator::MgbaEngine mgba(mgbaPath.toStdString());
            QVERIFY2(mgba.hasCore(), mgba.coreError().c_str());
            Pocket::Emulator::MelonDsEngine melon(melonPath.toStdString());
            QVERIFY(!melon.hasCore());
            QVERIFY(!melon.coreError().empty());
        }
        Pocket::Emulator::MelonDsEngine melon(melonPath.toStdString());
        QVERIFY2(melon.hasCore(), melon.coreError().c_str());
    }
};

QTEST_MAIN(TestLibretroCoreSession)
#include "test_libretro_core_session.moc"
