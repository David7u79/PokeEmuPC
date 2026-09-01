# ADR 0001: UI Framework Selection (Qt Widgets vs. Qt Quick)

- **Status**: Decided
- **Date**: 2026-09-01
- **Deciders**: PocketPartner Lead Architect

---

## Context and Problem Statement

PocketPartner requires a Windows-first desktop UI shell (main application `PocketPartner`) and a lightweight floating desktop companion widget (`PocketCompanion`). Key product invariants demand low memory usage, minimal idle CPU/GPU consumption, fast cold startup performance, and high battery efficiency on laptops.

We need to choose between **Qt Widgets** and **Qt Quick (QML)** as the primary UI framework.

---

## Comparison Matrix

| Evaluation Criterion | Qt Widgets | Qt Quick (QML) |
|---|---|---|
| **Memory Footprint** | **Low (~18 - 25 MB)**. Direct C++ class instances; minimal metadata overhead. | **Higher (~55 - 95 MB)**. QML engine, scene graph nodes, JavaScript V8 engine, template instantiations. |
| **Rendering Overhead** | **Event-driven (0 FPS idle)**. Paints strictly on user input or state change. Native OS double-buffering. | **Scene Graph Loop**. Default render loop requires active graphics context (D3D11/OpenGL) and frequent scene graph updates. |
| **Frameless Translucent Window Support** | **Excellent & Native**. `Qt::FramelessWindowHint` + `Qt::WA_TranslucentBackground` with `QPainter` has negligible overhead. | **Requires Graphics Context**. Translucent QQuickView requires alpha channel D3D swapchains with higher GPU power draw. |
| **Cold Startup Performance** | **Fast (< 150 ms)**. Direct binary symbol resolution without QML compilation / parsing phase. | **Slower (350 - 600 ms)**. Requires QML file parsing, type resolution, and JIT compiler initialization. |
| **Development Complexity** | **Low**. Strong C++ typing, compile-time signal/slot verification, unified debugging in MSVC. | **Medium-High**. Requires C++/QML context bindings, QQuickItem extensions, and asynchronous QML loading. |

---

## Decision Outcome

**Selected Framework**: **Qt Widgets** (`QtWidgets`)

### Rationale:
1. **Low-Power Idle Policy**: PocketCompanion must run at **0 FPS** when hidden or static. Qt Widgets naturally sleeps until an OS window event or custom repaint is dispatched.
2. **Resource Constraints**: PocketPartner prioritizes RAM usage, CPU usage, and battery life. Qt Widgets saves ~40-60 MB of RAM per process compared to Qt Quick.
3. **Frameless Desktop Companion Widget**: Transparent, click-drag desktop companion widgets are simpler, safer, and consume significantly less battery when implemented using C++ `QWidget` and `QPainter`.
