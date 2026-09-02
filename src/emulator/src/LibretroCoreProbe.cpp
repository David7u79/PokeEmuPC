#include "pocket/emulator/LibretroCoreProbe.hpp"
#include "pocket/emulator/Ilibretro.h"
#include <QFileInfo>
#include <QLibrary>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Pocket::Emulator {
namespace {

template <typename T>
T resolve(QLibrary& library, const char* name)
{
    return reinterpret_cast<T>(library.resolve(name));
}

std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

LibretroCoreDescription probeLibretroCore(const std::string& path)
{
    LibretroCoreDescription description;
    if (path.empty()) {
        description.error = "core library path is empty";
        return description;
    }

    const QFileInfo file(QString::fromStdString(path));
    if (!file.isFile()) {
        description.error = "core library file not found: " + path;
        return description;
    }

    QLibrary library(file.absoluteFilePath());
    if (!library.load()) {
        description.error = "could not load core library: " + library.errorString().toStdString();
        return description;
    }

    const auto unload = [&library]() {
        if (library.isLoaded())
            library.unload();
    };
    const auto fail = [&description, &unload](const char* symbol) {
        description.error = std::string("missing symbol ") + symbol;
        unload();
        return description;
    };

    const auto apiVersion = resolve<unsigned (*)()>(library, "retro_api_version");
    if (!apiVersion)
        return fail("retro_api_version");
    const auto getSystemInfo = resolve<void (*)(retro_system_info*)>(library, "retro_get_system_info");
    if (!getSystemInfo)
        return fail("retro_get_system_info");
    if (!resolve<void (*)()>(library, "retro_init"))
        return fail("retro_init");
    if (!resolve<bool (*)(const retro_game_info*)>(library, "retro_load_game"))
        return fail("retro_load_game");
    if (!resolve<void (*)()>(library, "retro_run"))
        return fail("retro_run");

    description.apiVersion = apiVersion();
    retro_system_info info{};
    getSystemInfo(&info);
    description.libraryName = info.library_name ? info.library_name : "";
    description.libraryVersion = info.library_version ? info.library_version : "";
    description.validExtensions = info.valid_extensions ? info.valid_extensions : "";
    description.needFullpath = info.need_fullpath;
    description.blockExtract = info.block_extract;
    description.valid = true;
    if (description.apiVersion != 1)
        description.error = "unsupported libretro API version " + std::to_string(description.apiVersion);
    unload();
    return description;
}

bool coreSupportsSystem(const LibretroCoreDescription& core, Core::GameSystem system)
{
    if (!core.valid)
        return false;

    std::string expected = Core::GameSystemUtils::defaultExtension(system);
    if (expected.empty())
        return false;
    expected.erase(0, 1);
    std::transform(expected.begin(), expected.end(), expected.begin(), [](unsigned char c) { return std::tolower(c); });

    std::istringstream extensions(core.validExtensions);
    std::string extension;
    while (std::getline(extensions, extension, '|')) {
        extension = trim(extension);
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (extension == expected)
            return true;
    }
    return false;
}

} // namespace Pocket::Emulator
