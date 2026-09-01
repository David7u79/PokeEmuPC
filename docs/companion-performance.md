# PocketCompanion Performance & Memory Measurements

- **Date**: 2026-09-01
- **Platform**: Windows 11 Home 64-bit
- **Architecture**: x86_64 / AMD64
- **Target**: `apps/companion/PocketCompanion.exe`
- **Compiler**: MSVC 19.38 (C++20)
- **UI Framework**: Qt Widgets (`QtWidgets` + `QPainter`)

---

## 1. Measured Performance Metrics

| Metric | Measured Empirical Value | Target / Requirement | Status |
|---|---|---|---|
| **Idle Memory (Working Set)** | **29.33 MB** | < 45.0 MB | **PASSED (Lightweight Footprint)** |
| **Idle CPU Usage** | **0.0 %** | 0.0 % (Near-zero sleeping) | **PASSED** |
| **Idle Rendering Framerate** | **0 FPS** | 0 FPS (Event-driven) | **PASSED** |
| **Window Drag Framerate** | **25 FPS** | 20-30 FPS (Interactive) | **PASSED** |
| **IPC Roundtrip Latency** | **< 0.1 ms** | < 5.0 ms | **PASSED** |

---

## 2. Process Separation & Lifecycle Decoupling

1. **No Emulator Cores Loaded**: `PocketCompanion.exe` links only `pocket_core`, `pocket_companion_core`, `pocket_storage`, and `PocketPartnerDesktopCompanion`. It does **not** load mGBA or melonDS emulator core libraries, preserving low memory usage (~29.3 MB).
2. **Independent Process Lifecycles**:
   - Closing `PocketPartner.exe` does NOT close `PocketCompanion.exe`.
   - Closing `PocketCompanion.exe` does NOT terminate `PocketPartner.exe`.
3. **Event-Driven Sleep Governance**:
   - `FramerateGovernor` keeps render timers stopped (0 FPS) while static or hidden.
   - Repaints occur strictly upon user window drag (`mouseMoveEvent`), system tray interaction, or IPC status changes.
