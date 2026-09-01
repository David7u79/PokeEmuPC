# Pokémon Sprite Asset Architecture & Directory Structure

This document details the sprite asset repository structure, file naming conventions, species indexing, shiny support, forms, gender variants, and local caching strategy for **PocketPartner**.

---

## 1. Upstream Repositories

### Primary Source: PokeSprite
- **Repository**: [PokeSprite](https://github.com/msikma/pokesprite)
- **Format**: Individual PNG files for Pokémon regular and shiny variants, items, and inventory icons.
- **Directory Structure**:
  - `assets/pokemon/pokesprite/regular/<slug>.png`
  - `assets/pokemon/pokesprite/shiny/<slug>.png`
  - `assets/pokemon/pokesprite/female/<slug>.png` (for gender-dimorphic species)

### Fallback Source: PKHeX Asset Set
- **Repository**: [PKHeX](https://github.com/kwsch/PKHeX)
- **Format**: Indexed PNG resources by species number (`001.png` .. `649.png`).
- **Directory Structure**:
  - `assets/pokemon/pkhex/regular/<speciesId>.png`
  - `assets/pokemon/pkhex/shiny/<speciesId>.png`

---

## 2. Species Mapping & Naming Convention

SpeciesId (1..649) is deterministically mapped to the lowercase PokeSprite slug (e.g., `1` -> `bulbasaur`, `25` -> `pikachu`, `197` -> `umbreon`, `384` -> `rayquaza`, `387` -> `turtwig`, `495` -> `snivy`).

### Resolution Hierarchy:
1. **Shiny Variant**:
   - `shiny/<slug>.png` (PokeSprite)
   - `shiny/<speciesId>.png` (PKHeX)
   - Fallback to Normal variant if shiny is unavailable.
2. **Gender Variant**:
   - `female/<slug>.png` if `gender == Gender::Female` and asset exists.
   - Fallback to Base regular variant.
3. **Form Variant**:
   - `<slug>-<formName>.png` (e.g. `unown-b.png`, `deoxys-attack.png`).
   - Fallback to Base regular variant.
4. **Placeholder**:
   - Procedural 64x64 silhouette icon if no file is found on disk.

---

## 3. Nearest-Neighbor Integer Scaling & LRU Cache

- **Pixel-Art Sharpness**: Scaled using `Qt::FastTransformation` (nearest-neighbor interpolation) to preserve crisp pixel edges at 100%, 125%, 150%, and 200% Windows DPI scaling.
- **In-Memory Cache (`SpriteCache`)**:
  - Capacity: 32 decoded `QPixmap` images.
  - Zero disk I/O during rendering frames.
