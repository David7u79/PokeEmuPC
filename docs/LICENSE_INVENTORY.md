# PocketPartner Upstream License Inventory & Legal Governance

This document records all upstream projects evaluated, referenced, or linked by **PocketPartner**, including their licenses, intended usage, and distribution obligations.

---

## 1. Upstream Project Inventory

| Project | Repository URL | Primary License | Permitted Usage in PocketPartner | Obligations & Restrictions |
|---|---|---|---|---|
| **mGBA** | https://github.com/mgba-emu/mgba | MPL 2.0 | Native emulator core reference / plugin dynamically linked core | Preserve copyright notices; changes to mGBA core source files must remain under MPL 2.0. Clean C++ abstraction layer keeps PocketPartner UI decoupled. |
| **melonDS** | https://github.com/melonDS-emu/melonDS | GPLv3 | Native DS emulator core reference / separate core process | GPLv3 obligations apply if linked into executable. Architected as out-of-process or decoupled C++ engine plugin. |
| **Lemuroid** | https://github.com/Swordfish90/Lemuroid | GPLv3 | Android architecture reference only | Do NOT copy source code. Reference for Android lifecycle & Libretro integration patterns. |
| **LibretroDroid** | https://github.com/Swordfish90/LibretroDroid | GPLv3 | Android JNI bridge reference only | Do NOT copy source code. Architecture reference only. |
| **PKHeX** | https://github.com/kwsch/PKHeX | GPLv3 | Save-format research, validation, and behavioral reference ONLY | **STRICT PROHIBITION**: Do NOT copy or port PKHeX C# source code into PocketPartner. Treat strictly as research specification for save offsets, checksum algorithms, and validation rules. |
| **SQLite** | https://sqlite.org | Public Domain | Embedded database engine for app-only companion state & local storage | None (Public Domain). |
| **Qt 6** | https://www.qt.io | LGPLv3 / Commercial | GUI framework, IPC, and event loop | Dynamic linking against Qt 6 libraries; source code relinking capability preserved. |
| **Catch2** | https://github.com/catchorg/Catch2 | BSL-1.0 | Unit test framework | Include standard Boost Software License 1.0 headers. |

---

## 2. Strict Code Import Rules

1. **No Proprietary Assets**:
   - Commercial ROMs, Nintendo BIOS/firmware, and copyrighted game sprites MUST NEVER be bundled, downloaded, or committed into the repository.
   - PocketPartner uses placeholders, neutral silhouettes, or user-provided assets.

2. **Clean Room Implementation**:
   - Save file parsing, 12-step mutation pipeline, and companion logic are original C++20 implementations built using public specifications and research notes.
   - PKHeX is used solely to verify expected semantic diffs during research and testing.

3. **Decoupled Architecture**:
   - `PocketPartnerCore` and `PocketCompanion.exe` do not depend directly on emulator core internals.
   - All emulator operations are routed through the abstract `EmulatorEngine` interface.
