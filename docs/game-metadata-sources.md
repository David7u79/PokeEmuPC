# Game Metadata & Artwork Sources Technical Specification

This document details the upstream metadata and thumbnail cover artwork integration for **PocketPartner**, including licensing boundaries, URL format specifications, local disk caching structure, and offline resilience strategies.

---

## 1. Upstream Data Sources

### Metadata Provider (Libretro Database)
- **Upstream Repository**: [Libretro Database](https://github.com/libretro/libretro-database)
- **Reference Tooling**: [libretrodb-sqlite](https://github.com/avojak/libretrodb-sqlite)
- **Licensing**: GPLv3 / MIT metadata definitions.
- **Coverage**:
  - `Nintendo - Game Boy.rdb`
  - `Nintendo - Game Boy Color.rdb`
  - `Nintendo - Game Boy Advance.rdb`
  - `Nintendo - Nintendo DS.rdb`

### Artwork Provider (Libretro Thumbnails)
- **Upstream Repositories**:
  - [Nintendo - Game Boy](https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy)
  - [Nintendo - Game Boy Color](https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color)
  - [Nintendo - Game Boy Advance](https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance)
  - [Nintendo - Nintendo DS](https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_DS)
- **Licensing Note**: Artwork licensing is separate from code and metadata licenses. Thumbnail images are aggregated from public domain and promotional game packaging. All artwork fetches are performed asynchronously on demand and cached locally.
- **Provider Abstraction**: PocketPartner uses a modular `ArtworkProvider` C++ interface to allow replacing the artwork provider dynamically if needed.

---

## 2. Remote URL Format Specification

Libretro thumbnails follow the raw GitHub URL pattern:
`https://raw.githubusercontent.com/libretro-thumbnails/<Platform>/master/<Category>/<CanonicalTitle>.png`

### Platforms
- `Nintendo_-_Game_Boy`
- `Nintendo_-_Game_Boy_Color`
- `Nintendo_-_Game_Boy_Advance`
- `Nintendo_-_Nintendo_DS`

### Artwork Categories
- `Named_Boxarts` (Primary Preference: `BOX_ART`)
- `Named_Titles` (Secondary Fallback: `TITLE_SCREEN`)
- `Named_Snaps` (Tertiary Fallback: `SCREENSHOT`)

---

## 3. Local Disk Cache Architecture

Artwork is NEVER stored as BLOB data inside SQLite. Images are stored on disk under the user's application data cache directory:

```
<AppData>/cache/artwork/
  <gameId>/
    boxart.png
    title.png
    screenshot.png
    thumb_128x128.png
```

### Negative Cache Ledger & Offline Resilience
To avoid repeated HTTP GET requests for missing artwork or during offline operation:
- `ArtworkCache` maintains a local `negative_cache.json` recording failed URLs and timestamp.
- Suppresses retries for 7 days unless explicitly cleared by the user in Settings.
