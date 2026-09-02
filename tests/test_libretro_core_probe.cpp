#include <QtTest>
#include <QTemporaryFile>
#include "DevAssets.hpp"
#include "pocket/emulator/LibretroCoreProbe.hpp"

using namespace Pocket;

class LibretroCoreProbeTest : public QObject {
    Q_OBJECT

private slots:
    void rejectsEmptyAndMissing();
    void rejectsNonCore();
    void probesMelonDsWhenAvailable();
    void probesMgbaWhenAvailable();
    void unloadsBetweenProbes();
};

void LibretroCoreProbeTest::rejectsEmptyAndMissing()
{
    const auto empty = Emulator::probeLibretroCore({});
    QVERIFY(!empty.valid);
    QVERIFY(!empty.error.empty());
    const auto missing = Emulator::probeLibretroCore("not-a-core-that-does-not-exist.dll");
    QVERIFY(!missing.valid);
    QVERIFY(!missing.error.empty());
}

void LibretroCoreProbeTest::rejectsNonCore()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write("not a dynamic library") > 0);
    file.flush();
    const auto description = Emulator::probeLibretroCore(file.fileName().toStdString());
    QVERIFY(!description.valid);
    QVERIFY(!description.error.empty());
}

void LibretroCoreProbeTest::probesMelonDsWhenAvailable()
{
    const QString core = DevAssets::melonDsCore();
    if (core.isEmpty())
        QSKIP("melonDS core is not configured");
    const auto description = Emulator::probeLibretroCore(core.toStdString());
    QVERIFY(description.valid);
    QVERIFY(QString::fromStdString(description.libraryName).contains("melonDS"));
    QVERIFY(!description.libraryVersion.empty());
    QVERIFY(QString::fromStdString(description.validExtensions).contains("nds"));
    QVERIFY(Emulator::coreSupportsSystem(description, Core::GameSystem::NDS));
    QVERIFY(!Emulator::coreSupportsSystem(description, Core::GameSystem::GBA));
}

void LibretroCoreProbeTest::probesMgbaWhenAvailable()
{
    const QString core = DevAssets::mgbaCore();
    if (core.isEmpty())
        QSKIP("mGBA core is not configured");
    const auto description = Emulator::probeLibretroCore(core.toStdString());
    QVERIFY(description.valid);
    QVERIFY(Emulator::coreSupportsSystem(description, Core::GameSystem::GBA));
    QVERIFY(!Emulator::coreSupportsSystem(description, Core::GameSystem::NDS));
}

void LibretroCoreProbeTest::unloadsBetweenProbes()
{
    const QString mgba = DevAssets::mgbaCore();
    const QString melonDs = DevAssets::melonDsCore();
    if (mgba.isEmpty() || melonDs.isEmpty())
        QSKIP("both real cores are required");
    QVERIFY(Emulator::probeLibretroCore(mgba.toStdString()).valid);
    QVERIFY(Emulator::probeLibretroCore(mgba.toStdString()).valid);
    QVERIFY(Emulator::probeLibretroCore(melonDs.toStdString()).valid);
}

QTEST_MAIN(LibretroCoreProbeTest)
#include "test_libretro_core_probe.moc"
