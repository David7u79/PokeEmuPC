# Generation III Pokémon Save File Format Documentation

This document describes the binary save file structure for Pokémon Generation III Game Boy Advance games (**Ruby**, **Sapphire**, **Emerald**, **FireRed**, and **LeafGreen**).

> [!IMPORTANT]
> **Licensing & Independent Implementation Notice**
> Upstream PKHeX is licensed under GPL-3.0. Information in this document is derived strictly from public format specifications and independent reverse engineering. No source code from PKHeX is copied into PocketPartner.

---

## 1. High-Level Memory Layout

Gen III save files are 128 KB (131,072 bytes) Flash memory dumps containing two redundant 56 KB save slots:

| Offset Range | Size | Component |
|---|---|---|
| `0x00000 - 0x0DFFF` | 57,344 bytes (56 KB) | **Save Slot A** (14 sections × 4,096 bytes) |
| `0x0E000 - 0x1BFFF` | 57,344 bytes (56 KB) | **Save Slot B** (14 sections × 4,096 bytes) |
| `0x1C000 - 0x1DFFF` | 8,192 bytes | Hall of Fame Data |
| `0x1E000 - 0x1FFFF` | 8,192 bytes | Mystery Gift / Trainer Hill / Recording Data |

Each 56 KB slot contains 14 sections (Section ID 0 through 13). Each section is 4,096 bytes, where data spans bytes `0x000 - 0xFFB` and the last 12 bytes (`0xFF4 - 0xFFF`) store metadata:

| Section Footer Offset | Size | Field |
|---|---|---|
| `0xFF4` | 2 bytes | Section ID (`0` to `13`) |
| `0xFF6` | 2 bytes | Section Checksum (16-bit sum of 32-bit words over data bytes `0x000 - 0xFF3`) |
| `0xFF8` | 4 bytes | Security Signature (`0x08012002` or game validation signature) |
| `0xFFC` | 4 bytes | Save Index Counter (incremented on each save write) |

---

## 2. Active Save Slot Selection Algorithm

1. Parse both **Slot A** (`0x00000`) and **Slot B** (`0x0E000`).
2. For each slot, iterate over all 14 sections:
   - Compute section checksum: sum of all 32-bit dwords from offset `0x000` to `0xFF0` added together as uint32, then upper 16 bits added to lower 16 bits.
   - Verify computed checksum against footer checksum at `0xFF6`.
   - Read 32-bit Save Index counter at `0xFFC`.
3. If all 14 sections of a slot have valid checksums, the slot is **Valid**.
4. If both Slot A and Slot B are valid, choose the slot with the **higher Save Index counter**.
5. If neither slot is fully valid, return a structured parse error (`InvalidChecksum`).

---

## 3. Section Mapping

The 14 sections are written to flash memory out of order. They are mapped by Section ID (`0..13`):

- **Section 0**: Trainer Info (Player Name, Gender, Trainer ID, Secret ID, Play Time, Money)
- **Section 1**: Party & Items (Party Count, 6 × 100-byte Party Pokémon structures)
- **Section 2**: Game State & Event Flags
- **Section 3**: Miscellaneous Data
- **Section 4**: Rival / Roaming Data
- **Section 5 - 13**: PC Storage Boxes (14 Boxes × 30 Boxed Pokémon = 420 Pokémon, 80 bytes each)

---

## 4. Pokémon Binary Structure (100-byte Party / 80-byte PC Box)

A party Pokémon consists of **100 bytes**:
- `0x00 - 0x4F` (80 bytes): Encrypted Box Pokémon data
- `0x50 - 0x63` (20 bytes): Unencrypted Battle Status data (Level, HP, Max HP, Stats, Status Condition)

### 4.1 Substructure Encryption & Shuffling

The 48 bytes from offset `0x20` to `0x4F` are divided into four 12-byte blocks:
- **Block G**: Growth (Species, Held Item, Experience, PP Bonuses, Friendship)
- **Block A**: Attacks (Moves 1-4, PPs)
- **Block E**: EVs & Condition (EVs HP, Atk, Def, SpA, SpD, Spe; Contest Stats)
- **Block M**: Miscellaneous (OT ID, IVs, Ribbons, Egg flag)

#### Encryption
Key = `Personality Value (PID) ^ Trainer ID (OT ID)`.
All 12 dwords from `0x20` to `0x4F` are XORed with `Key`.

#### Order Permutation
The order of the 4 blocks (G, A, E, M) is determined by `PID % 24`:

| `PID % 24` | Block Order | `PID % 24` | Block Order |
|---|---|---|---|
| 0 | GAEM | 12 | EGAM |
| 1 | GAME | 13 | EGMA |
| 2 | GEAM | 14 | EAGM |
| 3 | GEMA | 15 | EAMG |
| 4 | GMAE | 16 | EMGA |
| 5 | GMEA | 17 | EMAG |
| 6 | AGEM | 18 | MGAE |
| 7 | AGME | 19 | MGEA |
| 8 | AEGM | 20 | MAGE |
| 9 | AEMG | 21 | MAEG |
| 10 | AMGE | 22 | MEGA |
| 11 | AMEG | 23 | MEAG |

---

## 5. Derived Properties

- **Nature**: `PID % 25` (0 = Hardy, 1 = Lonely, 2 = Brave, 3 = Adamant, 4 = Naughty, ..., 24 = Quirky).
- **Unown Letter**: (For Unown) Derived from PID bit calculation.
- **Gender**: Determined by species base gender ratio compared to lower byte of PID (`PID & 0xFF`).
- **Shiny**: `(OT_ID ^ Secret_ID ^ (PID >> 16) ^ (PID & 0xFFFF)) < 8`.
