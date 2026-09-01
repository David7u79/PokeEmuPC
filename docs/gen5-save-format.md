# Pokémon Generation V Save File Format Specification

This document details the 512KB (`524,288` bytes) Flash save file structure for **Pokémon Black**, **White**, **Black 2**, and **White 2** (Nintendo DS).

---

## 1. High-Level 512KB Flash Memory Map

Gen V uses a block-based structure split across 512KB Flash:

| Offset Range | Size | Component / Purpose |
|---|---|---|
| `0x1F000 - 0x1F080` | 128 B | Party Pokémon Count & Structures |
| `0x00000 - 0x1EFFF` | 126,976 B | Main Game Block (Player, Bag, Pokedex) |
| `0x26000 - 0x53FFF` | 188,416 B | PC Storage Block (24 Boxes x 30 Slots = 720 Pokémon) |

- **Save Counter**: 32-bit integer evaluated to select active save block.
- **Checksum**: CRC-16-CCITT (`polynomial 0x1021`) computed over block data.

---

## 2. Gen V Pokémon Structure (220 Bytes)

Party Pokémon structures in Gen V are 220 bytes (136 bytes un-battle data + 84 bytes battle stats).

| Offset | Size | Field Name | Description |
|---|---|---|---|
| `0x00 - 0x03` | 4 B | Personality Value (PID) | 32-bit PRNG seed & identity |
| `0x04 - 0x05` | 2 B | Checksum | 16-bit CRC-16 checksum of decrypted blocks |
| `0x08 - 0x87` | 128 B | Encrypted Data Blocks | 4 x 32-byte blocks (Block A, B, C, D) |

---

## 3. Decryption & Unshuffling
- **PRNG Decryption**: Same LCRNG PRNG as Gen IV (`seed = checksum`, `seed = seed * 0x41C64E6D + 0x60B90885`).
- **Substructure Unshuffling**: 24 permutations calculated via `(PID & 0x3E) >> 1 % 24`.
- **Character Encoding**: Gen V uses native 16-bit UTF-16 Little Endian encoding for Nicknames (11 characters) and OT Names (7 characters).
