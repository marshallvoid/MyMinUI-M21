# MyMinUI

MyMinUI is a fork of [MinUI](https://github.com/shauninman/MinUI) that expands emulator coverage (arcade/FBN, DOOM, etc.), adds boxart support, and provides a Fancy Mode with live boxart and save-state previews.

> **Note**: This fork targets the **SJGAM M21** and **SJGAM M22 PRO**.

## Supported devices

- SJGAM M21
- SJGAM M22 PRO

## Documentation

- [Installation Guide](docs/INSTALL.md) — Install instructions for M21/M22 PRO
- [Usage Guide](docs/USAGE.md) — Shortcuts, features, modes, and tips
- [Boxart Guide](docs/BOXART.md) — Boxart locations, dimensions, and batch conversion
- [Cheats Guide](docs/CHEATS.md) — How to use cheats
- [Multidisc Guide](docs/MULTIDISC.md) — m3u, PBP, and CHD multidisc formats
- [Paks Guide](docs/PAKS.md) — Creating custom emulator and tool paks

## Releases

Latest release: https://github.com/Turro75/MyMinUI/releases

## Key differences from upstream MinUI

- Added arcade emulators: Final Burn Neo, NeoGeo, NeoGeoCD, MAME2003-PLUS
- Added standalone emulators: Prboom (DOOM), TyrQuake, Pico-8 (native), Wonderswan, Atari, DOS, Amiga
- PNG boxart support for both systems and ROMs
- Fancy Mode with boxart display and save-state previews
- Batch boxart converter tool
- Extended aspect ratio options (extended, force 4:3, force 3:2)
- Integrated cheats support
- Improved multidisc support (added PBP multidisc format)
- Rewritten CPU rendering engine (NEON, multicore, double-buffered)
- 256MB swapfile for larger games
