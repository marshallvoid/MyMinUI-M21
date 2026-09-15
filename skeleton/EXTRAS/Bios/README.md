# Bios

The `Bios/` directory under `EXTRAS/` contains BIOS files for **extended/emulator-specific systems** not in the core `BASE/Bios/` set.

## Structure

```
EXTRAS/Bios/
├── A2600/       # Atari 2600
├── A5200/       # Atari 5200
├── A7800/       # Atari 7800
├── DOOM/        # Doom (prboom.wad)
├── DOS/         # MS-DOS
├── FBN/         # Final Burn Neo
│   └── fbneo/   # FBN-specific BIOS files
├── GG/          # Sega Game Gear
├── MAME/        # Arcade (MAME 2003-Plus)
├── MGBA/        # Game Boy Advance
├── NG/          # NeoGeo
├── NGCD/        # NeoGeo CD
│   └── neocd/   # NeoGeo CD BIOS
├── NGPC/        # Neo Geo Pocket Color
├── P8/          # Pico-8 (requires purchased pico8.rp9 / pico8_dyn / pico8_64)
├── PCE/         # TurboGrafx-16
├── PKM/         # Pokemon Mini
├── PUAE/        # Amiga
├── QUAKE/       # Quake (id1/ contents)
├── SGB/         # Super Game Boy
├── SMS/         # Sega Master System
└── VB/          # Virtual Boy
```

## Usage

Copy BIOS files matching the folder tag. The emulator will look in `Bios/<TAG>/` for each system.

| System | Required Files |
|---|---|
| PS | `SCPH1001.BIN` (or `SCPH5501.BIN`) |
| NG | `neo-epo.zip`, `neo-po.zip`, `uni-bios.rom` |
| NGCD | `neocd` folder with NeoGeo CD BIOS |
| FBN | FBN BIOS files in `FBN/fbneo/` |
| P8 | `pico8.rp9` (Pico-8 data, must be purchased) |
| QUAKE | `id1/` folder with `pak0.pak` (shareware) or `pak1.pak` (full) |
| DOOM | `prboom.wad` |

Core system BIOS (GB, GBC, GBA, FC, SFC, MD, PS) are in `BASE/Bios/`.
