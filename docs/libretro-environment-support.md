# Libretro environment support

This table records the 28 environment commands observed from the melonDS DS
core by `test_melonds_core_probe`. `SUPPORTED` means Pocket provides the
requested value, `SAFE_NOOP` means the core's declaration is accepted without
runtime work, and `UNSUPPORTED` is returned as `false`.

| Command | Status | Reason |
| --- | --- | --- |
| SET_ROTATION | UNSUPPORTED | Pocket does not rotate the core framebuffer. |
| GET_CAN_DUPE | SUPPORTED | Duplicate frames are accepted. |
| SET_MESSAGE | UNSUPPORTED | No libretro message overlay is implemented. |
| SET_PERFORMANCE_LEVEL | SAFE_NOOP | It is advisory metadata only. |
| GET_SYSTEM_DIRECTORY | SUPPORTED | Returns the per-core temporary workspace. |
| SET_PIXEL_FORMAT | SUPPORTED | XRGB8888, RGB565 and 0RGB1555 are converted/accepted. |
| SET_INPUT_DESCRIPTORS | SAFE_NOOP | Pocket owns input labels and mappings. |
| SET_HW_RENDER | UNSUPPORTED | The renderer is software-framebuffer based. |
| GET_VARIABLE | SUPPORTED | Returns no override so the core uses its defaults. |
| SET_VARIABLES | SAFE_NOOP | Core option declarations need no local UI registration. |
| GET_VARIABLE_UPDATE | SUPPORTED | Always false; it is queried per frame and is never logged. |
| SET_SUPPORT_NO_GAME | SAFE_NOOP | No-game support declaration has no effect on loaded ROMs. |
| GET_LIBRETRO_PATH | UNSUPPORTED | The host does not disclose a libretro module path. |
| GET_LOG_INTERFACE | SUPPORTED | Provides the Qt debug logging callback. |
| GET_PERF_INTERFACE | UNSUPPORTED | No libretro performance counter interface is exposed. |
| GET_CORE_ASSETS_DIRECTORY | UNSUPPORTED | There is no shared core-assets directory contract. |
| GET_SAVE_DIRECTORY | SUPPORTED | Returns the per-core temporary workspace. |
| SET_SYSTEM_AV_INFO | SAFE_NOOP | Geometry/timing are refreshed from system AV info after load. |
| SET_SUBSYSTEM_INFO | UNSUPPORTED | Subsystem content loading is not implemented. |
| SET_CONTROLLER_INFO | SAFE_NOOP | The host already selects the joypad device. |
| SET_MEMORY_MAPS | SAFE_NOOP | Memory maps are informational for this host. |
| SET_GEOMETRY | SAFE_NOOP | Frame callbacks carry the authoritative geometry. |
| GET_USERNAME | UNSUPPORTED | Pocket supplies no core username. |
| GET_LANGUAGE | UNSUPPORTED | Pocket supplies no libretro language identifier. |
| GET_INPUT_BITMASKS | UNSUPPORTED | Input is polled by individual button id. |
| GET_CORE_OPTIONS_VERSION | SUPPORTED | Returns version 0 because no options UI is registered. |
| SET_CORE_OPTIONS | SAFE_NOOP | Option schema is accepted without a host options UI. |
| GET_MESSAGE_INTERFACE_VERSION | SUPPORTED | Returns version 0 because no extended message UI exists. |

Unknown commands are returned as unsupported and emitted once per command with
`qDebug()`, never from the frame-rate `GET_VARIABLE_UPDATE` path.
