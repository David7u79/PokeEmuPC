# Pokémon Generation IV Save File Format Specification

This document details the 512KB (`524,288` bytes) Flash save file structure for **Pokémon Diamond**, **Pearl**, **Platinum**, **HeartGold**, and **SoulSilver** (Nintendo DS).

---

## 1. High-Level 512KB Flash Memory Map

Gen IV uses a two-slot redundant save structure split into two main blocks:

| Block Name | Slot A Address Range | Slot B Address Range | Size | Purpose |
|---|---|---|---|---|
| **Small General Block** | `0x00000 - 0x0C0FF` | `0x0C100 - 0x181FF` | 49,408 B | Player Info, Party, Bag, Pokedex, Flags |
| **Large Storage Block** | `0x18200 - 0x3CF00` | `0x3CF00 - 0x61C00` | 150,784 B | PC Boxes 1-18 (540 Pokémon) |

- **Save Counter**: Located at offset `0xC0F0` (Small) and `0x3CEF0` (Large) as uint32_t. The slot with the higher save counter is the active save slot.
- **Checksum**: CRC-16-CCITT (`polynomial 0x1021`) computed over block data.

---

## 2. Gen IV Pokémon Structure (236 Bytes)

Party Pokémon structures in Gen IV are 236 bytes. Boxed Pokémon structures are 136 bytes (un-battle stats).

| Offset | Size | Field Name | Description |
|---|---|---|---|
| `0x00 - 0x03` | 4 B | Personality Value (PID) | 32-bit PRNG seed & identity |
| `0x04 - 0x05` | 2 B | Checksum | 16-bit CRC-16 checksum of decrypted blocks |
| `0x08 - 0x87` | 128 B | Encrypted Data Blocks | 4 x 32-byte blocks (Block A, B, C, D) |

---

## 3. LCRNG PRNG Decryption & Unshuffling

### Decryption Algorithm
Gen IV uses a Linear Congruential Random Number Generator (LCRNG) initialized with the 16-bit checksum stored at offset `0x06`:
- `seed = checksum`
- `seed = (seed * 0x41C64E6D + 0x60B90885) & 0xFFFFFFFF`
- `key = (seed >> 16) & 0xFFFF`
- XOR each 16-bit word in the 128-byte data block with `key`.

### Substructure Unshuffling
The 4 encrypted 32-byte blocks are shuffled into 1 of 24 permutations based on the Personality Value:
- `shiftValue = ((PID & 0x3E) >> 1) % 24`
- Unshuffle using the 24 standard Gen IV permutation patterns into canonical **A, B, C, D** order:
  - **Block A**: Species, Held Item, OT ID, Exp, Friendship, Ability.
  - **Block B**: Moves 1-4, Move PP, EVs, IVs, Contest Stats.
  - **Block C**: Nickname (11 UTF-16 characters).
  - **Block D**: OT Name (7 UTF-16 characters), Markings, Game of Origin.
