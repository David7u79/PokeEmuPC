# Pokémon Generation I Save File Format Specification

This document details the 32KB (`32,768` bytes) SRAM save file structure for **Pokémon Red**, **Blue**, and **Yellow** (Game Boy).

---

## 1. High-Level SRAM Memory Map

| Address Range | Size | Component / Purpose |
|---|---|---|
| `0x0000 - 0x1FFF` | 8,192 B | Hall of Fame Data |
| `0x2000 - 0x2597` | 1,432 B | System & Options State |
| `0x2598 - 0x25A2` | 11 B | Player Trainer Name |
| `0x2F2C` | 1 B | Party Pokémon Count (`0..6`) |
| `0x2F2D - 0x2F33` | 7 B | Party Species List (`0xFF` terminated) |
| `0x2F34 - 0x3037` | 264 B | Party Pokémon Structures (6 x 44 bytes) |
| `0x3038 - 0x307F` | 72 B | Party OT Trainer Names (6 x 12 bytes) |
| `0x3080 - 0x30C7` | 72 B | Party Nicknames (6 x 12 bytes) |
| `0x3523` | 1 B | Main Checksum Byte |

---

## 2. Checksum Algorithm
- Gen I uses a single 8-bit additive checksum located at `0x3523`.
- Sum all bytes from `0x2598` to `0x3522` as `uint8_t` sum (`sum = (sum + byte) & 0xFF`).
- The stored checksum at `0x3523` must equal `sum ^ 0xFF`.

---

## 3. Party Pokémon Structure (44 Bytes per Creature)

| Offset | Size | Field Name | Description |
|---|---|---|---|
| `0x00` | 1 B | Species ID | Gen I internal species ID |
| `0x01 - 0x02` | 2 B | Current HP | uint16_t big-endian |
| `0x03` | 1 B | Level | Level byte |
| `0x04` | 1 B | Status Condition | Sleep/Poison/Burn/Freeze/Paralyze bitfield |
| `0x05 - 0x06` | 2 B | Type 1 / Type 2 | Type IDs |
| `0x07` | 1 B | Catch Rate / Held Item | Internal catch rate byte |
| `0x08 - 0x0B` | 4 B | Moves 1-4 | Move IDs |
| `0x0C - 0x0D` | 2 B | Trainer ID | uint16_t OT ID |
| `0x0E - 0x10` | 3 B | Experience | 24-bit big-endian integer |
| `0x11 - 0x12` | 2 B | HP Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x13 - 0x14` | 2 B | Attack Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x15 - 0x16` | 2 B | Defense Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x17 - 0x18` | 2 B | Speed Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x19 - 0x1A` | 2 B | Special Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x1B - 0x1C` | 2 B | DVs (Determinant Values) | Packed 4-bit DVs: Atk/Def (Byte 1), Spe/Spc (Byte 2) |
| `0x1D - 0x20` | 4 B | Move PP 1-4 | Current PP for moves |
| `0x21` | 1 B | Level (Redundant) | Level byte |
| `0x22 - 0x23` | 2 B | Max HP | uint16_t big-endian |
| `0x24 - 0x25` | 2 B | Attack Stat | uint16_t big-endian |
| `0x26 - 0x27` | 2 B | Defense Stat | uint16_t big-endian |
| `0x28 - 0x29` | 2 B | Speed Stat | uint16_t big-endian |
| `0x2A - 0x2B` | 2 B | Special Stat | uint16_t big-endian |

---

## 4. Determinant Values (DVs)
- **Attack DV**: `(Byte 1 >> 4) & 0x0F`
- **Defense DV**: `Byte 1 & 0x0F`
- **Speed DV**: `(Byte 2 >> 4) & 0x0F`
- **Special DV**: `Byte 2 & 0x0F`
- **HP DV**: Derived from LSBs of (Atk DV % 2 << 3) | (Def DV % 2 << 2) | (Spe DV % 2 << 1) | (Spc DV % 2).
