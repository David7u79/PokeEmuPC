// Core identity and environment discovery for the melonDS DS libretro core.
//
// Deliberately standalone: it talks to the DLL directly rather than through
// MelonDsEngine, so it verifies the core itself and reports what the core
// actually asks for. Everything the engine implements is derived from what
// this prints, not from assumptions about mGBA.

#include <QtTest/QtTest>
#include <QCryptographicHash>
#include <QLibrary>
#include <QMap>
#include <QTemporaryDir>
#include <atomic>
#include "DevAssets.hpp"
#include "pocket/emulator/Ilibretro.h"

namespace {

struct Probe {
    QMap<unsigned, int> envCalls;     // cmd -> times requested
    QMap<unsigned, bool> envAnswers;  // cmd -> what we answered the first time
    int videoCallbacks{0};
    int audioBatches{0};
    qint64 audioFrames{0};
    unsigned width{0}, height{0};
    size_t pitch{0};
    QList<QByteArray> frameHashes;
    QByteArray systemDir, saveDir;
    int pixelFormat{-1};
};

Probe* g_probe = nullptr;

bool environmentCallback(unsigned cmd, void* data)
{
    if (!g_probe) return false;
    g_probe->envCalls[cmd] += 1;

    bool answer = false;
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        if (data) {
            g_probe->pixelFormat = *static_cast<const int*>(data);
            answer = g_probe->pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888
                  || g_probe->pixelFormat == RETRO_PIXEL_FORMAT_RGB565;
        }
        break;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        if (data) { *static_cast<const char**>(data) = g_probe->systemDir.constData(); answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (data) { *static_cast<const char**>(data) = g_probe->saveDir.constData(); answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        if (data) { *static_cast<bool*>(data) = true; answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        // Let the core fall back to its own defaults.
        if (data) { static_cast<retro_variable*>(data)->value = nullptr; answer = false; }
        break;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (data) { *static_cast<bool*>(data) = false; answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        if (data) { *static_cast<unsigned*>(data) = 0; answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
        if (data) { *static_cast<unsigned*>(data) = 0; answer = true; }
        break;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        answer = false; // report plain per-id polling
        break;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        answer = true; // accepted, nothing for a probe to store
        break;
    default:
        answer = false; // never blanket-true: unsupported must read as unsupported
        break;
    }

    if (!g_probe->envAnswers.contains(cmd)) g_probe->envAnswers[cmd] = answer;
    return answer;
}

void videoCallback(const void* data, unsigned width, unsigned height, size_t pitch)
{
    if (!g_probe) return;
    ++g_probe->videoCallbacks;
    g_probe->width = width;
    g_probe->height = height;
    g_probe->pitch = pitch;
    if (!data || width == 0 || height == 0) return;

    // Sample every 60th frame so the hash list stays small.
    if (g_probe->videoCallbacks % 60 != 1) return;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    const auto* bytes = static_cast<const char*>(data);
    for (unsigned y = 0; y < height; ++y) hash.addData(QByteArrayView(bytes + y * pitch, static_cast<qsizetype>(width * 4)));
    g_probe->frameHashes.append(hash.result());
}

size_t audioBatchCallback(const int16_t*, size_t frames)
{
    if (g_probe) { ++g_probe->audioBatches; g_probe->audioFrames += static_cast<qint64>(frames); }
    return frames;
}

void audioSampleCallback(int16_t, int16_t) {}
void inputPollCallback() {}
int16_t inputStateCallback(unsigned, unsigned, unsigned, unsigned) { return 0; }

const char* envName(unsigned cmd)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_ROTATION: return "SET_ROTATION";
    case RETRO_ENVIRONMENT_GET_CAN_DUPE: return "GET_CAN_DUPE";
    case RETRO_ENVIRONMENT_SET_MESSAGE: return "SET_MESSAGE";
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: return "SET_PERFORMANCE_LEVEL";
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: return "GET_SYSTEM_DIRECTORY";
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: return "SET_PIXEL_FORMAT";
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: return "SET_INPUT_DESCRIPTORS";
    case RETRO_ENVIRONMENT_SET_HW_RENDER: return "SET_HW_RENDER";
    case RETRO_ENVIRONMENT_GET_VARIABLE: return "GET_VARIABLE";
    case RETRO_ENVIRONMENT_SET_VARIABLES: return "SET_VARIABLES";
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: return "GET_VARIABLE_UPDATE";
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: return "SET_SUPPORT_NO_GAME";
    case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: return "GET_LIBRETRO_PATH";
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: return "GET_LOG_INTERFACE";
    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: return "GET_PERF_INTERFACE";
    case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY: return "GET_CORE_ASSETS_DIRECTORY";
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: return "GET_SAVE_DIRECTORY";
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: return "SET_SYSTEM_AV_INFO";
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: return "SET_SUBSYSTEM_INFO";
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: return "SET_CONTROLLER_INFO";
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: return "SET_MEMORY_MAPS";
    case RETRO_ENVIRONMENT_SET_GEOMETRY: return "SET_GEOMETRY";
    case RETRO_ENVIRONMENT_GET_USERNAME: return "GET_USERNAME";
    case RETRO_ENVIRONMENT_GET_LANGUAGE: return "GET_LANGUAGE";
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return "GET_INPUT_BITMASKS";
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: return "GET_CORE_OPTIONS_VERSION";
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: return "SET_CORE_OPTIONS";
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: return "SET_CORE_OPTIONS_INTL";
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: return "SET_CORE_OPTIONS_DISPLAY";
    case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION: return "GET_MESSAGE_INTERFACE_VERSION";
    case RETRO_ENVIRONMENT_SET_MESSAGE_EXT: return "SET_MESSAGE_EXT";
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: return "GET_AUDIO_VIDEO_ENABLE";
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: return "SET_CORE_OPTIONS_V2";
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: return "SET_CORE_OPTIONS_V2_INTL";
    case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE: return "SET_CONTENT_INFO_OVERRIDE";
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT: return "GET_GAME_INFO_EXT";
    default: return "";
    }
}

} // namespace

class TestMelonDsCoreProbe : public QObject {
    Q_OBJECT
private slots:
    void probeRealCore()
    {
        const QString corePath = DevAssets::melonDsCore();
        const QString romPath = DevAssets::ndsRom();
        if (corePath.isEmpty()) QSKIP("melonDS DS core not found: set POCKET_MELONDSDS_CORE or drop melondsds_libretro.dll in the discovery dir");
        if (romPath.isEmpty()) QSKIP("no .nds ROM found: set POCKET_NDS_ROM or drop one in the discovery dir");

        QLibrary library(corePath);
        QVERIFY2(library.load(), qPrintable(library.errorString()));

        auto sym = [&library](const char* name) { return library.resolve(name); };
        auto get_system_info = reinterpret_cast<void(*)(retro_system_info*)>(sym("retro_get_system_info"));
        auto set_environment = reinterpret_cast<void(*)(retro_environment_t)>(sym("retro_set_environment"));
        auto init = reinterpret_cast<void(*)()>(sym("retro_init"));
        auto deinit = reinterpret_cast<void(*)()>(sym("retro_deinit"));
        auto set_video = reinterpret_cast<void(*)(retro_video_refresh_t)>(sym("retro_set_video_refresh"));
        auto set_audio_batch = reinterpret_cast<void(*)(retro_audio_sample_batch_t)>(sym("retro_set_audio_sample_batch"));
        auto set_audio = reinterpret_cast<void(*)(retro_audio_sample_t)>(sym("retro_set_audio_sample"));
        auto set_input_poll = reinterpret_cast<void(*)(retro_input_poll_t)>(sym("retro_set_input_poll"));
        auto set_input_state = reinterpret_cast<void(*)(retro_input_state_t)>(sym("retro_set_input_state"));
        auto load_game = reinterpret_cast<bool(*)(const retro_game_info*)>(sym("retro_load_game"));
        auto unload_game = reinterpret_cast<void(*)()>(sym("retro_unload_game"));
        auto run = reinterpret_cast<void(*)()>(sym("retro_run"));
        auto get_av_info = reinterpret_cast<void(*)(retro_system_av_info*)>(sym("retro_get_system_av_info"));
        auto get_memory_data = reinterpret_cast<void*(*)(unsigned)>(sym("retro_get_memory_data"));
        auto get_memory_size = reinterpret_cast<size_t(*)(unsigned)>(sym("retro_get_memory_size"));
        auto set_port_device = reinterpret_cast<void(*)(unsigned, unsigned)>(sym("retro_set_controller_port_device"));

        QVERIFY(get_system_info && set_environment && init && load_game && run && get_av_info);

        // --- identity: never trust the filename ---
        retro_system_info info{};
        get_system_info(&info);
        const QString libraryName = QString::fromUtf8(info.library_name ? info.library_name : "");
        qInfo().noquote() << "library_name      :" << libraryName;
        qInfo().noquote() << "library_version   :" << (info.library_version ? info.library_version : "");
        qInfo().noquote() << "valid_extensions  :" << (info.valid_extensions ? info.valid_extensions : "");
        qInfo().noquote() << "need_fullpath     :" << info.need_fullpath;
        qInfo().noquote() << "block_extract     :" << info.block_extract;
        QVERIFY2(libraryName.contains("melon", Qt::CaseInsensitive),
                 qPrintable("not a melonDS core: " + libraryName));

        // Writable workspace: the developer's files stay untouched.
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        Probe probe;
        const QString systemDir = DevAssets::libretroSystemDir().isEmpty() ? workspace.path() : DevAssets::libretroSystemDir();
        probe.systemDir = QFile::encodeName(systemDir);
        probe.saveDir = QFile::encodeName(workspace.path());
        g_probe = &probe;

        set_environment(environmentCallback);
        init();
        set_video(videoCallback);
        if (set_audio) set_audio(audioSampleCallback);
        if (set_audio_batch) set_audio_batch(audioBatchCallback);
        if (set_input_poll) set_input_poll(inputPollCallback);
        if (set_input_state) set_input_state(inputStateCallback);

        // need_fullpath cores read the file themselves; otherwise hand over bytes.
        QByteArray romBytes;
        retro_game_info game{};
        const QByteArray romPathUtf8 = QFile::encodeName(romPath);
        game.path = romPathUtf8.constData();
        if (!info.need_fullpath) {
            QFile romFile(romPath);
            QVERIFY(romFile.open(QIODevice::ReadOnly));
            romBytes = romFile.readAll();
            game.data = romBytes.constData();
            game.size = static_cast<size_t>(romBytes.size());
        }

        const bool loaded = load_game(&game);
        qInfo().noquote() << "retro_load_game   :" << (loaded ? "accepted" : "REJECTED");
        QVERIFY2(loaded, "core rejected the ROM");

        if (set_port_device) set_port_device(0, RETRO_DEVICE_JOYPAD);

        retro_system_av_info av{};
        get_av_info(&av);
        qInfo().noquote() << "geometry          :" << av.geometry.base_width << "x" << av.geometry.base_height
                          << "max" << av.geometry.max_width << "x" << av.geometry.max_height
                          << "aspect" << av.geometry.aspect_ratio;
        qInfo().noquote() << "fps               :" << av.timing.fps;
        qInfo().noquote() << "sample_rate       :" << av.timing.sample_rate;
        QVERIFY(av.timing.fps > 0.0);
        QVERIFY(av.timing.sample_rate > 0.0);

        // --- real execution, uncapped ---
        QElapsedTimer timer;
        timer.start();
        const int frameCount = 1000;
        for (int i = 0; i < frameCount; ++i) run();
        const qint64 elapsed = timer.elapsed();

        qInfo().noquote() << "pixel_format      :" << probe.pixelFormat;
        qInfo().noquote() << "video callbacks   :" << probe.videoCallbacks;
        qInfo().noquote() << "video geometry    :" << probe.width << "x" << probe.height << "pitch" << probe.pitch;
        qInfo().noquote() << "audio batches     :" << probe.audioBatches << "frames" << probe.audioFrames;
        qInfo().noquote() << "uncapped throughput:" << (frameCount * 1000.0 / qMax<qint64>(1, elapsed)) << "fps";

        QStringList requested;
        for (auto it = probe.envCalls.cbegin(); it != probe.envCalls.cend(); ++it) {
            const QString name = QString::fromLatin1(envName(it.key()));
            requested << QString("%1(%2)x%3=%4")
                             .arg(name.isEmpty() ? QString("UNKNOWN") : name)
                             .arg(it.key())
                             .arg(it.value())
                             .arg(probe.envAnswers.value(it.key()) ? "true" : "false");
        }
        qInfo().noquote() << "environment asked :" << requested.join(", ");

        if (get_memory_data && get_memory_size) {
            qInfo().noquote() << "SAVE_RAM size     :" << get_memory_size(RETRO_MEMORY_SAVE_RAM)
                              << "ptr" << (get_memory_data(RETRO_MEMORY_SAVE_RAM) ? "yes" : "null");
            qInfo().noquote() << "SYSTEM_RAM size   :" << get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
        }

        QVERIFY2(probe.videoCallbacks > 0, "core produced no video callbacks");
        QVERIFY(probe.width > 0 && probe.height > 0);

        // Emulation must actually progress, not repaint one still frame.
        QVERIFY2(probe.frameHashes.size() >= 2, "not enough sampled frames");
        bool changed = false;
        for (int i = 1; i < probe.frameHashes.size(); ++i) {
            if (probe.frameHashes[i] != probe.frameHashes.first()) { changed = true; break; }
        }
        QVERIFY2(changed, "every sampled frame was identical: the core is not emulating");

        if (unload_game) unload_game();
        if (deinit) deinit();
        g_probe = nullptr;
        library.unload();
    }
};

QTEST_MAIN(TestMelonDsCoreProbe)
#include "test_melonds_core_probe.moc"
