# ADR 0002: Local Inter-Process Communication (IPC) Mechanism

- **Status**: Decided
- **Date**: 2026-09-01
- **Deciders**: PocketPartner Lead Architect

---

## Context and Problem Statement

PocketPartner uses a two-process architecture:
- `PocketPartner.exe`: Main desktop application (emulator host, library, settings, save safety coordinator).
- `PocketCompanion.exe`: Lightweight, event-driven desktop companion widget running in system tray / overlay.

We need a low-latency, event-driven, local Inter-Process Communication (IPC) mechanism to exchange status updates, window control commands, and companion activity events without loading emulator cores in `PocketCompanion.exe` or coupling UI thread loops.

We evaluated **Qt LocalSocket (`QLocalServer` / `QLocalSocket`)** versus **Direct Windows Named Pipes (`CreateNamedPipe` / `ReadFile` / `WriteFile`)**.

---

## Comparison Matrix

| Criteria | `QLocalServer` / `QLocalSocket` | Direct Windows Named Pipes |
|---|---|---|
| **Underlying OS Mechanism** | Windows Named Pipe (`\\.\pipe\...`) on Windows; Unix Domain Socket on Linux. | Windows Named Pipe (`\\.\pipe\...`). |
| **Qt Event Loop Integration** | **Native**. Non-blocking signal/slot events (`readyRead`, `connected`, `disconnected`). | Requires custom `OVERLAPPED` I/O or background worker thread loops with synchronization. |
| **IPC Message Roundtrip Latency** | **< 0.1 ms** (identical performance). | **< 0.1 ms**. |
| **Development Complexity & Safety** | **Low**. Managed connection lifecycle, automatic cleanup, no handle leaks. | **High**. Manual `HANDLE` management, security descriptors, thread locks. |
| **Cross-Platform Architecture** | **Seamless**. Identical C++ code compiles on Linux without platform branching. | Windows-only API (requires separate Linux socket implementation). |

---

## Decision Outcome

**Selected Mechanism**: **Qt LocalSocket (`QLocalServer` / `QLocalSocket`)** with length-prefixed JSON framing (`uint32_t` payload length header followed by versioned JSON message).

### Message Protocol & Versioning
- **Framing**: `[4-byte big-endian uint32_t payload_length][UTF-8 JSON string]`
- **Schema Version**: `version: 1`
- **Supported IPC Command Types**:
  1. `CompanionStatusRequest`
  2. `CompanionStatusChanged`
  3. `OpenMainApplication`
  4. `HideCompanion`
  5. `ShowCompanion`
  6. `ShutdownCompanion`
  7. `Ping` / `Pong`

### Rules:
- Raw SQLite database handles and binary pointers MUST NEVER be transmitted over IPC.
- Messages must use explicit versioned schemas.
- Automatic reconnection with exponential backoff is enforced when one process restarts.
