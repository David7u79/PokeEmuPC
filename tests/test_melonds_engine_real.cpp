#include <QtTest/QtTest>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <atomic>
#include "DevAssets.hpp"
#include "pocket/emulator/MelonDsEngine.hpp"

class TestMelonDsEngineReal : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { if (DevAssets::melonDsCore().isEmpty()) QSKIP("melonDS DS core not found: set POCKET_MELONDSDS_CORE or provide the developer asset"); if (DevAssets::ndsRom().isEmpty()) QSKIP("no .nds ROM found: set POCKET_NDS_ROM or provide the developer asset"); }
private:
    static std::unique_ptr<Pocket::Emulator::MelonDsEngine> makeEngine() {
        const QString core = DevAssets::melonDsCore(); const QString rom = DevAssets::ndsRom();
        if (core.isEmpty() || rom.isEmpty()) return nullptr;
        auto engine = std::make_unique<Pocket::Emulator::MelonDsEngine>(core.toStdString());
        if (!engine->hasCore() || !engine->loadRom(rom.toStdString())) return nullptr;
        return engine;
    }
private slots:
    void coreIdentity() { auto e=makeEngine(); QVERIFY(e); QVERIFY(e->systemInfo().libraryName.find("melon")!=std::string::npos); QVERIFY(!e->systemInfo().libraryVersion.empty()); }
    void bootsAndProducesFrames() { auto e=makeEngine(); std::atomic<int> count{}; int w=0,h=0; size_t pitch=0; e->setVideoFrameCallback([&](const uint8_t*,int x,int y,size_t p){w=x;h=y;pitch=p;++count;}); e->start(); QTRY_VERIFY_WITH_TIMEOUT(count.load()>120,5000); QCOMPARE(w,256); QCOMPARE(h,384); QCOMPARE(pitch,(size_t)w*4); e->stop(); }
    void framesChangeOverTime() { auto e=makeEngine(); QList<QByteArray> hashes; std::atomic<int> count{}; e->setVideoFrameCallback([&](const uint8_t*p,int w,int h,size_t pitch){const int n=++count;if(n%100)return;QCryptographicHash hash(QCryptographicHash::Sha1);for(int y=0;y<h;++y)hash.addData(QByteArrayView(reinterpret_cast<const char*>(p)+(size_t)y*pitch,(qsizetype)w*4));hashes.append(hash.result());}); for(int i=0;i<1000;++i)e->runFrameUnpaced(); QVERIFY(hashes.size()>=2); bool changed=false;for(int i=1;i<hashes.size();++i)if(hashes[i]!=hashes[0])changed=true;QVERIFY(changed); }
    void dualScreenSplit() { auto e=makeEngine(); std::atomic<int> count{}; e->setVideoFrameCallback([&](const uint8_t*,int,int,size_t){++count;}); e->start(); QTRY_VERIFY_WITH_TIMEOUT(count.load()>10,5000); const auto* top=e->topFramebuffer();const auto* bottom=e->bottomFramebuffer();QVERIFY(top);QVERIFY(bottom);QCOMPARE(e->screenFramebufferSize(),(size_t)256*192*4);QVERIFY(std::memcmp(top,bottom,e->screenFramebufferSize())!=0);e->stop(); }
    void audioArrives() { auto e=makeEngine(); std::atomic<int> batches{},frames{};e->setAudioSampleCallback([&](const int16_t*,size_t f){++batches;frames+=(int)f;});e->start();QTRY_VERIFY_WITH_TIMEOUT(batches.load()>10&&frames.load()>0,5000);QVERIFY(e->sampleRate()>0);e->stop(); }
    void realtimePacing() { auto e=makeEngine();std::atomic<int> frames{};e->setVideoFrameCallback([&](const uint8_t*,int,int,size_t){++frames;});e->start();QTest::qWait(2100);const int before=frames.load();QElapsedTimer timer;timer.start();QTest::qWait(2000);const double fps=(frames.load()-before)*1000.0/timer.elapsed();qInfo()<<"sustained fps:"<<fps;QVERIFY(fps>55.0);e->stop(); }
    void coreThroughput() { auto e=makeEngine();QElapsedTimer timer;timer.start();for(int i=0;i<500;++i)e->runFrameUnpaced();const double fps=500000.0/qMax<qint64>(1,timer.elapsed());qInfo()<<"uncapped throughput:"<<fps<<"fps";QVERIFY(fps>55.0); }
    void buttonsReachTheCore() { auto e=makeEngine(); const std::pair<Pocket::Emulator::EmulatorButton,unsigned> buttons[]={{Pocket::Emulator::EmulatorButton::A,RETRO_DEVICE_ID_JOYPAD_A},{Pocket::Emulator::EmulatorButton::B,RETRO_DEVICE_ID_JOYPAD_B},{Pocket::Emulator::EmulatorButton::X,RETRO_DEVICE_ID_JOYPAD_X},{Pocket::Emulator::EmulatorButton::Y,RETRO_DEVICE_ID_JOYPAD_Y},{Pocket::Emulator::EmulatorButton::L,RETRO_DEVICE_ID_JOYPAD_L},{Pocket::Emulator::EmulatorButton::R,RETRO_DEVICE_ID_JOYPAD_R},{Pocket::Emulator::EmulatorButton::Start,RETRO_DEVICE_ID_JOYPAD_START},{Pocket::Emulator::EmulatorButton::Select,RETRO_DEVICE_ID_JOYPAD_SELECT},{Pocket::Emulator::EmulatorButton::Up,RETRO_DEVICE_ID_JOYPAD_UP},{Pocket::Emulator::EmulatorButton::Down,RETRO_DEVICE_ID_JOYPAD_DOWN},{Pocket::Emulator::EmulatorButton::Left,RETRO_DEVICE_ID_JOYPAD_LEFT},{Pocket::Emulator::EmulatorButton::Right,RETRO_DEVICE_ID_JOYPAD_RIGHT}};for(const auto& b:buttons){e->sendButtonEvent(b.first,true);QCOMPARE(e->onInputState(0,RETRO_DEVICE_JOYPAD,0,b.second),int16_t(1));e->sendButtonEvent(b.first,false);QCOMPARE(e->onInputState(0,RETRO_DEVICE_JOYPAD,0,b.second),int16_t(0));} }
    void saveRamIsReachable() { auto e=makeEngine();QCOMPARE(e->getPersistentSave().size(),(size_t)524288); }
};
QTEST_MAIN(TestMelonDsEngineReal)
#include "test_melonds_engine_real.moc"
