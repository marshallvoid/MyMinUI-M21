# EXTRAS

The `EXTRAS/` directory contains user-facing content that ships with MyMinUI. On the SD card, these become top-level folders (`Bios/`, `Roms/`, `Cheats/`, `Imgs/`, `Emus/`, `Tools/`).

## Structure

```
EXTRAS/
├── Bios/               # BIOS files for extended/emulator-specific systems
├── Cheats/             # Cheat files (.cht) organized by system
├── Collections/        # Custom game collections (text file lists)
├── Emus/               # Emulator packages (.pak) — one per system
│   └── m21/
├── Imgs/               # Top-level boxart images (Tools, Favorites, etc.)
├── Roms/               # ROM directories for extended systems (with Imgs/)
└── Tools/              # Utility tools (.pak) — one per tool
    └── m21/
```

## Key Paths on SD Card

| Skeleton Path | SD Card Path | Description |
|---|---|---|
| `EXTRAS/Bios/` | `/mnt/SDCARD/Bios/` | Extended system BIOS files |
| `EXTRAS/Cheats/` | `/mnt/SDCARD/Cheats/` | RetroArch-style `.cht` files |
| `EXTRAS/Collections/` | `/mnt/SDCARD/Collections/` | Custom game collection `.txt` files |
| `EXTRAS/Emus/m21/*.pak/` | `/mnt/SDCARD/.system/m21/paks/Emus/*.pak/` | Emulator launch packages |
| `EXTRAS/Imgs/` | `/mnt/SDCARD/Imgs/` | Top-level icons for tools and menu items |
| `EXTRAS/Roms/` | `/mnt/SDCARD/Roms/` | Extended system ROM folders |
| `EXTRAS/Tools/m21/*.pak/` | `/mnt/SDCARD/.system/m21/paks/Tools/*.pak/` | Utility tool packages |

> **Note**: The `BASE/` directory contains the core systems (GB, GBC, GBA, FC, SFC, MD, PS). `EXTRAS/` contains additional systems added by MyMinUI.
