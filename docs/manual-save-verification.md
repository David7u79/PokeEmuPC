# Manual Save Verification Protocol (PKHeX & mGBA)

This document provides a developer protocol for manually verifying save file mutations performed by **PocketPartner** against third-party tools (**PKHeX** and **mGBA**).

> [!IMPORTANT]
> **Safety Notice**
> Always verify mutations using a developer-owned copy of a save file. Never perform testing directly on primary save backups.

---

## 6-Step Verification Protocol

### Step 1: Baseline Recording in PKHeX
1. Open **PKHeX.exe** (Reference tool).
2. Load the target Gen III save file (`.sav`).
3. Select the target Pokémon in Party or PC Box (e.g. Treecko).
4. Record baseline values:
   - **Friendship**: e.g., `92`
   - **EVs**: (e.g., HP 0, Atk 0, Def 0, SpA 0, SpD 0, Spe 0)
   - **IVs**: (e.g., HP 31, Atk 31, Def 31)
   - **Checksum Status**: Valid

### Step 2: PocketPartner Friendship Mutation
1. Open **PocketPartner.exe**.
2. Select the same Pokémon as the active companion.
3. Perform a friendship reward action (e.g., Pet or Feed).
4. Confirm mutation result in PocketPartner (e.g., Friendship changed `92 -> 94`).

### Step 3: Re-inspection in PKHeX
1. Re-open the modified `.sav` file in **PKHeX**.
2. Inspect the target Pokémon:
   - **Friendship**: Verify value matches updated target (e.g., `94`).
   - **Unrelated Fields**: Confirm EVs, IVs, Moves, Species, Level, PID, and Trainer ID remain 100% UNCHANGED.
   - **Save Integrity**: Verify PKHeX displays green **Valid Save Checksum**.

### Step 4: Loading in mGBA Emulator
1. Open **mGBA.exe**.
2. Load game ROM (Ruby, Sapphire, Emerald, FireRed, or LeafGreen) with the modified `.sav` file.
3. Verify game loads cleanly without displaying *"The save file is corrupted"* error.

### Step 5: In-Game Stats Verification
1. Check the Pokémon's stats and summary screen in-game.
2. Visit the Friendship Rater NPC (e.g. in Verdanturf Town or Pallet Town) to confirm in-game dialogue matches the increased friendship level.

### Step 6: Backup Audit Verification
1. Verify that `SaveBackupRepository` created a pre-mutation backup file in `backups/` directory prior to save file overwrite.
