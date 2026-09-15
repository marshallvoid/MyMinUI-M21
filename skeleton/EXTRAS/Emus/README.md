# Emulator Packages (Emus)

Each `.pak` directory in `EXTRAS/Emus/m21/` defines an emulator core. The `.pak` folder contains at minimum a `launch.sh` script and optionally a `default.cfg` with core-specific configuration.

## How It Works

1. `launch.sh` calls `bin/launch_rom.sh` (from `SYSTEM/m21/bin/`) which handles the actual ROM loading.
2. `launch.sh` specifies:
   - `EMU_EXE` — the libretro core name (e.g., `pcsx_rearmed`)
   - `RUN` / `RUN2` — frontend to use (typically `minarch`)
   - `CPU_OC` — CPU overclock level for this emulator

## Example: `PS.pak/launch.sh`

```sh
#!/bin/sh
EMU_EXE=pcsx_rearmed
RUN=minarch
RUN2=minarch
CPU_OC=${CPU_SPEED_PERF}
CURDIR="$(dirname "$0")"
launch_rom.sh "$1" "$2" "$3" "$4" ${RUN} ${RUN2} "$CURDIR" "$EMU_EXE" $CPU_OC
overclock.elf ${CPU_SPEED_MENU}
```

## `default.cfg`

Some `.pak` directories include a `default.cfg` file with core options. Lines prefixed with `-` are locked (user cannot change them from the in-game menu).

## Supported Emulators (M21)

| Tag | System | Core |
|---|---|---|
| GB | Game Boy | gambatte |
| GBC | Game Boy Color | gambatte |
| GBA | Game Boy Advance | mgba |
| FC | Nintendo Entertainment System | fceumm |
| SFC | Super Nintendo | snes9x2005_plus |
| MD | Sega Genesis | picodrive |
| PS | PlayStation | pcsx_rearmed |
| NG | NeoGeo | fbneo |
| NGCD | NeoGeo CD | fbneo |
| FBN | Final Burn Neo | fbneo |
| MAME | Arcade (MAME) | mame2003-plus |
| DOOM | Doom | prboom |
| P8 | Pico-8 | fake-08 |
| P8N | Pico-8 Native | pico8_dyn (requires purchased binary) |
| QUAKE | Quake | tyrquake |
| DOS | MS-DOS | dosbox |
| PUAE | Amiga | puae |
| SGB | Super Game Boy | snes9x2005_plus |
| SMS | Sega Master System | picodrive |
| GG | Sega Game Gear | picodrive |
| PCE | TurboGrafx-16 | mednafen_pce |
| VB | Virtual Boy | beetle-vb |
| NGPC | Neo Geo Pocket Color | mednafen_wonderswan |
| PKM | Pokemon Mini | pokemini |
| A2600 | Atari 2600 | stella |
| A5200 | Atari 5200 | a5200 |
| A7800 | Atari 7800 | prosystem |
