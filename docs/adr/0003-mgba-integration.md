# ADR 0003: mGBA Emulator Core Integration Strategy

- **Status**: Decided
- **Date**: 2026-09-01
- **Deciders**: Architecture Team
- **Context**: Milestone requirement to implement the first real GBA emulator integration (`MgbaEngine`) handling video rendering, audio output, keyboard/gamepad input, and persistent cartridge saves (`PersistentGameSave`).

---

## Evaluation of Integration Approaches

### Option A: Libretro mGBA Dynamic Native Core (`mgba_libretro.dll`) — SELECTED
- **Performance**: Zero-copy/minimal-copy 60 FPS video delivery (240x160 GBA native resolution rendered to RGBA32 texture/image). Audio buffered via lock-free queue to `QAudioSink`.
- **Maintainability**: Standardized `libretro` C API (`retro_init`, `retro_load_game`, `retro_run`, `retro_get_memory_data`, `retro_unload_game`). Clean decoupling between host UI and emulator engine.
- **Licensing**: mGBA is MPL-2.0. Dynamically loaded plugin core ensures strict license boundaries.
- **Save Access**: Direct pointer to SRAM/Flash/EEPROM persistent save memory via `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)`.
- **Process Unloading**: Loaded dynamically via `QLibrary` / `LoadLibraryW` when starting a GBA game; fully unloaded (`FreeLibrary`) on game stop. `PocketCompanion.exe` never loads the core library.

### Option B: Direct `libmgba` Core C API (`mCore`)
- **Performance**: High.
- **Maintainability**: Lower due to deep mGBA internal header dependencies and tight static linking.
- **Process Unloading**: Harder to unload cleanly if statically linked into host executable.

### Option C: Launch Standalone Subprocess (`mgba.exe`)
- **Performance**: Inter-process window embedding overhead.
- **Maintainability**: Low. Fragmented window management and audio synchronization issues on Windows.

---

## Decision

We select **Option A**: Implement `MgbaEngine` implementing the `EmulatorEngine` interface, loading the native mGBA core via the standardized libretro C interface.

### Architectural Invariants
1. **Abstraction**: The Qt UI layer communicates strictly with `EmulatorEngine` (`start()`, `pause()`, `resume()`, `stop()`, `sendButtonEvent()`, `savePersistentData()`).
2. **Process Independence**: `PocketCompanion.exe` remains 100% free of emulator cores.
3. **Save Distinction**:
   - `PersistentGameSave`: Cartridge SRAM/Flash/EEPROM saved to `.sav` files.
   - `SaveState`: Emulator RAM/CPU snapshot saved to `.ss1` files.
   - The two concepts are explicitly distinct in code and docs.
4. **Save Safety**: Never overwrite a valid persistent `.sav` file with 0-byte or uninitialized memory on emulator initialization.
