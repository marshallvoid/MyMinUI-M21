# Roms (Extended Systems)

The `EXTRAS/Roms/` directory contains ROM folders for **extended systems** that MyMinUI adds beyond the base MinUI support. These are organized by system tag, matching the emulator `.pak` tags in `EXTRAS/Emus/m21/`.

## Structure

Each system folder follows the naming convention `Full System Name (TAG)`:

```
EXTRAS/Roms/
├── Arcade (MAME)/          # MAME 2003-Plus
├── Atari5200 (A5200)/      # Atari 5200
├── Atari7800 (A7800)/      # Atari 7800
├── Doom (DOOM)/            # PrBoom (doom.wad, .wad files)
├── FinalBurnNeo (FBN)/     # Final Burn Neo
├── Game Boy Advance (MGBA)/# Game Boy Advance (via mgba core)
├── MSDOS (DOS)/            # DOSBox
├── Neo Geo Pocket Color (NGPC)/
├── NeoGeo (NG)/            # NeoGeo (via FBN)
├── NeoGeo CD (NGCD)/       # NeoGeo CD (via FBN)
├── Nintendo DS (NDS)/      # (if DS core is available)
├── Pico-8 (P8)/            # Pico-8 carts (.p8.png)
├── Pico-8 Native (P8N)/    # Native Pico-8 (binary)
├── Pokémon mini (PKM)/     # Pokemon Mini
├── Quake (QUAKE)/          # TyrQuake
├── Sega Game Gear (GG)/    # Game Gear
├── Sega Master System (SMS)/
├── Super Game Boy (SGB)/   # Super Game Boy
├── Super Nintendo Entertainment System (SUPA)/
├── TurboGrafx-16 (PCE)/    # TurboGrafx-16
└── Virtual Boy (VB)/       # Virtual Boy
```

## Boxart

Each ROM folder has an `Imgs/` subfolder for boxart images. PNG boxart files should be named after the ROM's display name (without extension), e.g.:

```
Roms/Arcade (MAME)/Imgs/1942.png
```

## Multi-Disc Games

For multi-disc games (PlayStation, Sega CD, etc.), use `.m3u` playlist files:

```
Roms/Sony PlayStation (PS)/Game Name/
├── Disc 1.bin
├── Disc 2.bin
└── Game Name.m3u
```

The `.m3u` file lists disc files, one per line. When a `.m3u` exists, individual `.bin`/`.cue` files are hidden — only the playlist is shown as a single selectable entry.
