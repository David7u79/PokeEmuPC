# Companion Power & Performance Architecture

This document describes the power management architecture, energy awareness mechanisms, and animation framerate policies for **PocketCompanion.exe**.

---

## 1. Primary Design Requirement

For a desktop widget that runs continuously alongside work applications, **CPU and battery efficiency take strict priority over animation complexity**.

- Continuous 60 FPS repaints consume 3–8% CPU and significantly reduce laptop battery runtime.
- PocketCompanion uses an **event-driven, variable-framerate animation controller** that defaults to **0 FPS when static or hidden**.

---

## 2. Animation Framerate Policies (`CompanionAnimationController`)

| State | Framerate (FPS) | Timer Interval | Trigger Condition |
|---|---|---|---|
| **HIDDEN** | **0 FPS** | Timer Stopped | Widget hidden or minimized |
| **STATIC** | **0 FPS** | Timer Stopped | No user interaction / static display |
| **SLOW_IDLE** | **6 FPS** | ~166 ms | Normal idle state when visible |
| **INTERACTION** | **25 FPS** | 40 ms | Temporary 1.5s boost during Feed/Pet/Train/Play |

---

## 3. Windows Battery Saver & Power Awareness

PocketCompanion queries the native Windows Power Management API via `GetSystemPowerStatus()` (`<winbase.h>` / `<windows.h>`):

- **Battery Saver Active** (`SystemStatusFlag & 0x01`): Automatically forces **0 FPS (STATIC)** mode, disabling non-essential idle animations.
- **On Battery Power** (`ACLineStatus == 0`): Reduces `SLOW_IDLE` framerate to **3 FPS**.
- **On AC Power**: Normal 6 FPS idle.

---

## 4. Startup Management (`CompanionStartupManager`)

- Autostart is managed cleanly via the per-user Windows Registry key:
  `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` -> `PocketPartnerCompanion`
- **Zero Administrator Privileges Required**: Writes strictly to `HKEY_CURRENT_USER`.
- **User Preference Enforced**: Autostart is OFF by default and enabled only upon explicit user selection.
