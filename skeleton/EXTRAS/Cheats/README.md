# Cheats

The `Cheats/` directory contains cheat files in RetroArch/NextUI `.cht` format, organized by system.

## Structure

```
EXTRAS/Cheats/
├── A2600/       # Atari 2600
├── A5200/       # Atari 5200
├── A7800/       # Atari 7800
├── DOOM/        # Doom
├── DOS/         # MS-DOS
├── FBN/         # Final Burn Neo
├── GG/          # Sega Game Gear
├── MAME/        # Arcade (MAME 2003-Plus)
├── MGBA/        # Game Boy Advance
├── NG/          # NeoGeo
├── NGCD/        # NeoGeo CD
├── NGPC/        # Neo Geo Pocket Color
├── P8/          # Pico-8
├── PCE/         # TurboGrafx-16
├── PKM/         # Pokemon Mini
├── PUAE/        # Amiga
├── QUAKE/       # Quake
├── SGB/         # Super Game Boy
├── SMS/         # Sega Master System
└── VB/          # Virtual Boy
```

## Cheat File Naming

Cheat files are matched by the ROM's display name (without extension). For example, a ROM `Super Example World (USA).zip` will look for:
1. `Cheats/GB/Super Example World (USA).cht`
2. `Cheats/GB/Super Example World.cht` (using alias from `map.txt` if available)

## Cheat File Format

`.cht` files use the RetroArch/NextUI format:

```
cheats = 2
cheat0_desc = "Infinite Lives"
cheat0_code = "12345678"
cheat0_enable = true
cheat1_desc = "Invincibility"
cheat1_code = "87654321"
cheat1_enable = false
```

## In-Game Usage

In the minarch in-game menu, navigate to the **Cheats** section to toggle cheats on/off.
