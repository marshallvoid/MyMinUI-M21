# Skeleton

This directory contains the template structure for MyMinUI releases. Contents are copied into `build/` by the `makefile` and then packaged into a release zip.

## Structure

```
skeleton/
├── BASE/          # Root SD card files (Bios, Roms, README)
├── SYSTEM/        # Internal system files (.system) — users normally don't modify
└── EXTRAS/        # User content: Bios, Roms, Cheats, Emus, Tools
```

## Build Process

1. `make setup` — copy skeleton into `build/`
2. `make common` — compile minui, minarch, and tools
3. `make system` — copy platform binaries (m21)
4. `make cores` — copy compiled libretro cores
5. `make package` — assemble into `releases/MyMinUI-YYYYMMDDb-N-m21.zip`

## For Users

After installing MyMinUI, the SD card structure mirrors this skeleton. Users populate `EXTRAS/` (Roms, Bios, Cheats) and `BASE/` (Bios, Roms). See the README.md in each directory for details.
