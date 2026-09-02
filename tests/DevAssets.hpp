#pragma once

// Developer-local test asset discovery. TEST INFRASTRUCTURE ONLY — production
// code must never learn about anyone's Desktop.
//
// Resolution order, first hit wins:
//   1. explicit environment variable
//   2. developer-local discovery directory (POCKET_DEV_ASSET_DIR, else the
//      well-known local folder below)
//   3. nothing — the caller QSKIPs with a reason
//
// Assets found this way are immutable inputs: never write to them. Use
// DevAssets::workspace() for anything an emulator will modify.

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

namespace DevAssets {

inline QString discoveryDir()
{
    const QString override = qEnvironmentVariable("POCKET_DEV_ASSET_DIR");
    if (!override.isEmpty()) return override;
    return QDir::homePath() + "/Desktop/Roms";
}

// Env var first, then the first file in the discovery dir matching any glob.
inline QString find(const char* envVar, const QStringList& patterns)
{
    const QString fromEnv = qEnvironmentVariable(envVar);
    if (!fromEnv.isEmpty()) return QFileInfo(fromEnv).isFile() ? fromEnv : QString();

    QDir dir(discoveryDir());
    if (!dir.exists()) return {};
    const auto entries = dir.entryInfoList(patterns, QDir::Files, QDir::Name);
    return entries.isEmpty() ? QString() : entries.first().absoluteFilePath();
}

inline QString melonDsCore() { return find("POCKET_MELONDSDS_CORE", {"melondsds_libretro.dll", "melondsds_libretro.so"}); }
inline QString mgbaCore()    { return find("POCKET_MGBA_CORE", {"mgba_libretro.dll", "mgba_libretro.so"}); }
inline QString ndsRom()      { return find("POCKET_NDS_ROM", {"*.nds"}); }
// "*.nds.sav" is what MelonDsEngine writes; plain "*.sav" would also match a GBA save.
inline QString ndsSave()     { return find("POCKET_NDS_SAVE", {"*.nds.sav", "*.dsv", "*.srm"}); }

// The 4-character game code at offset 0x0C of an NDS header. Identifies a title
// regardless of what the file was renamed to.
inline QString ndsGameCode(const QString& romPath)
{
    QFile rom(romPath);
    if (!rom.open(QIODevice::ReadOnly) || !rom.seek(0x0C)) return {};
    const QByteArray code = rom.read(4);
    return code.size() == 4 ? QString::fromLatin1(code) : QString();
}

// Generation IV cartridges: Diamond, Pearl, Platinum, HeartGold, SoulSilver.
// The trailing character is the region, so only the first three matter.
inline bool isGen4GameCode(const QString& code)
{
    static const QStringList kGen4{"ADA", "APA", "CPU", "IPK", "IPG"};
    return code.size() >= 3 && kGen4.contains(code.left(3));
}

// A Gen IV ROM paired with the save PocketPartner writes next to it. Both must
// exist: the save is the fixture, and it only appears once a session has saved.
inline QPair<QString, QString> gen4RomAndSave()
{
    const QString romOverride = qEnvironmentVariable("POCKET_GEN4_ROM");
    const QString saveOverride = qEnvironmentVariable("POCKET_GEN4_SAVE");
    if (!romOverride.isEmpty() && !saveOverride.isEmpty()) return {romOverride, saveOverride};

    QDir dir(discoveryDir());
    if (!dir.exists()) return {};
    for (const QFileInfo& rom : dir.entryInfoList({"*.nds"}, QDir::Files, QDir::Name)) {
        if (!isGen4GameCode(ndsGameCode(rom.absoluteFilePath()))) continue;
        const QString save = rom.absoluteFilePath() + ".sav";
        if (QFileInfo(save).isFile()) return {rom.absoluteFilePath(), save};
    }
    return {};
}

// Libretro system dir for BIOS/firmware. Empty when the developer has none:
// melonDS DS boots retail carts without them, so this is not a hard blocker.
inline QString libretroSystemDir()
{
    const QString fromEnv = qEnvironmentVariable("POCKET_LIBRETRO_SYSTEM_DIR");
    if (!fromEnv.isEmpty()) return fromEnv;
    QDir dir(discoveryDir());
    const auto bios = dir.entryInfoList({"bios7.bin", "bios9.bin", "firmware.bin"}, QDir::Files);
    return bios.isEmpty() ? QString() : dir.absolutePath();
}

} // namespace DevAssets
