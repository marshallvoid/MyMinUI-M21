# Boxart Guide & Batch Converters for MyMinUI

## Screen & Layout Dimensions (Fancy Mode)

MyMinUI Fancy Mode displays game lists on the left (X = 0..256, width 256) and boxart on the right (X = 256..640, width 384, height 480).

Target boxart dimensions for full-height right column alignment:

- `SW = 640`, `SH = 480`
- `BX = 256`, `BY = 0`, `BW = 384`, `BH = 480`
- `ASPECT = 0` (preserves original aspect ratio, centered in the right box)

---

## Batch Convert on Handheld (`Convert BoxArt.pak`)

1. Open **Options** (`Y` key) -> **Convert BoxArt**.
2. The tool scans all `Imgs` folders, backs up originals to `.bak`, and outputs converted PNGs in place.
3. Config files: `skeleton/EXTRAS/Tools/m21/Convert BoxArt.pak/toolboxart.cfg`.

---

## Boxart Locations (Lookup Order)

- ROMs: `/Roms/<EMU>/Imgs/<game>.png`
- Emulators: `/Roms/<EMU>/Imgs/<EMU>.png`
- Tools: `/Tools/m21/<pak>/Imgs/<pak>.png`
- System folders: `/Imgs/<folder>.png` (e.g. `/Imgs/Favorites.png`)
- Fallback: `/Imgs/default.png`
