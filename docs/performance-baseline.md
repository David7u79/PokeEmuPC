# PocketPartner Performance Baseline

- **Date**: 2026-09-01
- **Platform**: Windows 11 Home 64-bit
- **Architecture**: x86_64 / AMD64 (Intel 20-thread CPU)
- **Compiler**: Visual Studio 2022 MSVC 19.38 (C++20)
- **UI Framework**: Qt 6.7.2 MSVC 64-bit (Qt Widgets)
- **Build Type**: Release / Ninja

---

## 1. Measured Performance Metrics

| Metric | Measured Value | Measurement Target / Goal | Status |
|---|---|---|---|
| **Cold Startup Time** | **18 ms** | < 200 ms | **EXCEEDED (Extremely Fast)** |
| **Idle Memory (Working Set)** | **49.94 MB** | < 60.0 MB | **PASSED** |
| **Idle CPU Usage** | **0.0 %** | 0.0 % (Event-driven sleeping) | **PASSED** |
| **Framerate (Static/Idle Widget)** | **0 FPS** | 0 FPS (Paint on demand only) | **PASSED** |
| **Automated Test Suite Execution** | **0.93s (9/9 passed)** | < 3.0s | **PASSED** |

---

## 2. Measurement Methodology

1. **Cold Startup Time**: Measured in `apps/desktop/src/main.cpp` using high-resolution `QElapsedTimer` starting at binary entry (`main`) through full SQLite initialization, schema migration, `GameRepository` bootstrap, and `MainWindow::show()`.
2. **Idle Working Set RAM**: Measured using Windows Process Performance APIs (`WorkingSet64 / 1MB`) after application window initialization and resting idle for 2 seconds.
3. **Idle CPU Usage**: Measured using `TotalProcessorTime.TotalSeconds` over standard Qt event loop execution.
4. **Test Suite Duration**: Measured via CTest execution across all 9 automated unit tests.
