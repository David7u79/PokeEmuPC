#include "pocket/emulator/LibretroCoreSession.hpp"
#include <mutex>

namespace Pocket::Emulator {
namespace {
std::mutex s_sessionMutex;
LibretroEngineBase* s_owner = nullptr;
} // namespace

bool LibretroCoreSession::acquire(LibretroEngineBase* engine) {
    std::lock_guard<std::mutex> lock(s_sessionMutex);
    if (!engine || (s_owner && s_owner != engine))
        return false;
    s_owner = engine;
    return true;
}

void LibretroCoreSession::release(LibretroEngineBase* engine) {
    std::lock_guard<std::mutex> lock(s_sessionMutex);
    if (s_owner == engine)
        s_owner = nullptr;
}

LibretroEngineBase* LibretroCoreSession::current() {
    std::lock_guard<std::mutex> lock(s_sessionMutex);
    return s_owner;
}

bool LibretroCoreSession::isHeldBy(const LibretroEngineBase* engine) {
    std::lock_guard<std::mutex> lock(s_sessionMutex);
    return s_owner == engine;
}
} // namespace Pocket::Emulator
