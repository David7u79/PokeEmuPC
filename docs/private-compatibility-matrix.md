# Private compatibility matrix

Real-core compatibility targets that run against a developer's own dumps. These
tests never ship a ROM, a save or a core: they discover assets locally through
`tests/DevAssets.hpp` and SKIP with an explicit reason when nothing is found, so
the suite stays green on a machine that has none of them.

Nothing in this table belongs in portable CI.

## Cores

| System | Core | Version verified | Notes |
|---|---|---|---|
| GB / GBC / GBA | `mgba_libretro.dll` | — | 60 fps sustained, 65536 Hz |
| NDS | `melondsds_libretro.dll` | melonDS DS 1.3.1 | 256x384 XRGB8888, 59.8261 fps, 32728.5 Hz, no external BIOS |

## Titles

| Generation | Title | Status |
|---|---|---|
| Gen III | Pokémon Emerald (GBA) | Boots, 60 fps, SRAM round-trip verified |
| Gen IV | Pokémon Platinum (NDS) | Current NDS target |
| Gen V | Pokémon Black 2 (NDS) | **Future.** Deliberately out of scope until the NDS engine is stable; the same real-core suite should eventually run both. |

## Save fixtures

| Fixture | Present | Consequence |
|---|---|---|
| GBA `.sav` | yes | Gen III paths exercised against a real save |
| NDS save | **no** | `Gen4EmulationSaveIntegrationTest` is built but SKIPPED: "No developer-local Gen IV save fixture found". Do not fabricate a commercial-game save to turn it green — the point of the test is that a real emulator session produced the bytes. |

The NDS save fixture appears on its own once a session plays far enough to
trigger an in-game save; nothing needs to be authored by hand.

## Environment variables

Every discovery path can be overridden explicitly, which is how a second machine
or a CI runner with its own assets would opt in:

`POCKET_MGBA_CORE`, `POCKET_TEST_ROM`, `POCKET_MELONDSDS_CORE`, `POCKET_NDS_ROM`,
`POCKET_NDS_SAVE`, `POCKET_LIBRETRO_SYSTEM_DIR`, `POCKET_DEV_ASSET_DIR`.
