# Pokémon Generation II Save File Format Specification

This document details the 32KB (`32,768` bytes) SRAM save file structure for **Pokémon Gold**, **Silver**, and **Crystal** (Game Boy Color).

---

## 1. High-Level SRAM Memory Map

| Address Range | Size | Component / Purpose |
|---|---|---|
| `0x2000 - 0x200A` | 11 B | Player Trainer Name |
| `0x2865` | 1 B | Party Pokémon Count (`0..6`) |
| `0x2866 - 0x286C` | 7 B | Party Species List (`0xFF` terminated) |
| `0x286D - 0x298C` | 288 B | Party Pokémon Structures (6 x 48 bytes) |
| `0x298D - 0x29D4` | 72 B | Party OT Trainer Names (6 x 12 bytes) |
| `0x29D5 - 0x2A1C` | 72 B | Party Nicknames (6 x 12 bytes) |
| `0x2B83 - 0x2B84` | 2 B | Primary Checksum Word (uint16_t big-endian) |
| `0x0C00 - 0x1782` | 2,947 B | Secondary Save Slot Backup |
| `0x1783 - 0x1784` | 2 B | Secondary Checksum Word (uint16_t big-endian) |

---

## 2. Checksum Algorithm
- Gen II uses 16-bit additive checksums.
- For Primary Slot: Sum all bytes from `0x2000` to `0x2B82` as 16-bit unsigned integer.
- The 16-bit stored checksum at `0x2B83` (big-endian) must equal the 16-bit sum.

---

## 3. Party Pokémon Structure (48 Bytes per Creature)

| Offset | Size | Field Name | Description |
|---|---|---|---|
| `0x00` | 1 B | Species Index | Gen II species index |
| `0x01` | 1 B | Held Item | Item ID |
| `0x02 - 0x05` | 4 B | Moves 1-4 | Move IDs |
| `0x06 - 0x07` | 2 B | Trainer ID | uint16_t OT ID |
| `0x08 - 0x0A` | 3 B | Experience | 24-bit big-endian integer |
| `0x0B - 0x0C` | 2 B | HP Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x0D - 0x0E` | 2 B | Attack Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x0F - 0x10` | 2 B | Defense Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x11 - 0x12` | 2 B | Speed Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x13 - 0x14` | 2 B | Special Stat Exp | 16-bit big-endian (`0..65535`) |
| `0x15 - 0x16` | 2 B | DVs (Determinant Values) | Packed 4-bit DVs: Atk/Def (Byte 1), Spe/Spc (Byte 2) |
| `0x17 - 0x1A` | 4 B | Move PP 1-4 | Current PP for moves |
| `0x1B` | 1 B | Friendship | Gen II Friendship byte (`0..255`) |
| `0x1C` | 1 B | Pokerus Status | Pokerus byte |
| `0x1D - 0x1E` | 2 B | Caught Data | Level/Time/Location caught |
| `0x1F` | 1 B | Level | Level byte |
| `0x20` | 1 B | Status Condition | Sleep/Poison/Burn/Freeze/Paralyze bitfield |
| `0x22 - 0x23` | 2 B | Current HP | uint16_t big-endian |
| `0x24 - 0x25` | 2 B | Max HP | uint16_t big-endian |
| `0x26 - 0x27` | 2 B | Attack Stat | uint16_t big-endian |
| `0x28 - 0x29` | 2 B | Defense Stat | uint16_t big-endian |
| `0x2A - 0x2B` | 2 B | Speed Stat | uint16_t big-endian |
| `0x2C - 0x2D` | 2 B | Special Attack Stat | uint16_t big-endian |
| `0x2E - 0x2F` | 2 B | Special Defense Stat | uint16_t big-endian |
