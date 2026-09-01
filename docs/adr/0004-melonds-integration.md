# ADR 0004: Nintendo DS Emulation Integration Strategy (melonDS)

## Status
Accepted

## Context
PocketPartner requires Nintendo DS emulation support for Game Boy Advance and Nintendo DS Pokémon titles. Upstream `melonDS` (https://github.com/melonDS-emu/melonDS) is the standard open-source Nintendo DS emulator.

`melonDS` is licensed under the **GNU General Public License v3 (GPLv3)**. We must carefully evaluate architectural approaches to ensure licensing boundaries are respected, core resources unload cleanly when NDS emulation stops, and `PocketCompanion.exe` remains completely lightweight and independent of emulator code.

## Integration Approaches Evaluated

### Option A: In-Process Native C++ Embedding Engine (`MelonDsEngine`)
- **Pros**: Direct memory access to 256x192 dual screen framebuffers, sub-millisecond input responsiveness, native Qt Widget rendering integration.
- **Cons**: Requires clean C++ abstraction layer (`EmulatorEngine`) to avoid contaminating UI components with melonDS internal headers.
- **Licensing**: Fully compliant when entire `PocketPartner.exe` repository is open source under GPLv3. `PocketCompanion.exe` links only `pocket_companion_core` and contains ZERO emulator code!

### Option B: Libretro melonDS Core Integration
- **Pros**: Standardized Libretro API for video, audio, input.
- **Cons**: Adds libretro dynamic loading complexity, additional dependency overhead.

### Option C: Subprocess / Out-of-Process Execution
- **Pros**: Complete process isolation.
- **Cons**: Video stream IPC marshalling overhead for dual 256x192 screens.

## Decision
We select **Option A**: Implement an in-process native `MelonDsEngine` inheriting `EmulatorEngine` within `PocketPartner.exe`.

Key Architectural Constraints:
1. `PocketCompanion.exe` MUST NOT link melonDS code or libretro cores under any circumstances.
2. `MelonDsEngine` must cleanly release all video, audio, and CPU memory resources when `stop()` or `unload()` is invoked.
3. System BIOS files (`bios7.bin`, `bios9.bin`) and Firmware (`firmware.bin`) must NEVER be distributed or automatically downloaded. They are user-configured external dependencies.
4. Save file management must strictly reuse `SaveSessionCoordinator` and `SaveBackupRepository` to enforce single-writer save locking.

## Consequences
- Clean separation between core emulator engines (`mGBA`, `melonDS`) and application UI.
- Dual-screen rendering (Vertical, Horizontal, Focused) and touchscreen mouse input handled cleanly in `NdsDisplayWidget`.
- Zero memory leakage when switching between GBA and NDS titles in `PocketPartner.exe`.
