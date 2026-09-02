#pragma once

namespace Pocket::Emulator {
class LibretroEngineBase;

// Libretro callbacks and core internals are process-global: exactly one core may be active at a time.
class LibretroCoreSession {
public:
    static bool acquire(LibretroEngineBase* engine);
    static void release(LibretroEngineBase* engine);
    static LibretroEngineBase* current();
    static bool isHeldBy(const LibretroEngineBase* engine);
};
} // namespace Pocket::Emulator
