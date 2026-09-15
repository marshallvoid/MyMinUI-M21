# BASE

The `BASE/` directory contains the core system OS files — these end up at the **root of the SD card** after installation.

## Structure

```
BASE/
├── README.txt              # Detailed install instructions
└── Bios/
    ├── FC/                 # BIOS for Nintendo Entertainment System
    ├── GB/                 # BIOS for Game Boy
    ├── GBA/                # BIOS for Game Boy Advance
    ├── GBC/                # BIOS for Game Boy Color
    ├── MD/                 # BIOS for Sega Genesis
    └── PS/                 # BIOS for PlayStation (SCPH1001.BIN, SCPH5501.BIN, ...)
└── Roms/
    ├── Game Boy (GB)/
    │   └── Imgs/           # Directory for game boxart images
    ├── Game Boy Advance (GBA)/
    │   └── Imgs/
    ├── Game Boy Color (GBC)/
    │   └── Imgs/
    ├── Nintendo Entertainment System (FC)/
    │   └── Imgs/
    ├── Sega Genesis (MD)/
    │   └── Imgs/
    └── Sony PlayStation (PS)/
        └── Imgs/
```

## Usage

### BIOS
- Copy system BIOS files into the matching subdirectory.
- Example: For PlayStation, copy `SCPH1001.BIN` into `BASE/Bios/PS/`.
- For extended systems (NeoGeo, Arcade, etc.), BIOS files live in `EXTRAS/Bios/`.

### Roms
- The `Roms/` directory holds game ROMs organized by system.
- Folder names follow the format `System Name (TAG)` — e.g., `Sony PlayStation (PS)`.
- The `Imgs/` subfolder in each system holds default boxart images.

> **Note**: BASE directories are copied directly to the SD card root. Maintain the structure.
