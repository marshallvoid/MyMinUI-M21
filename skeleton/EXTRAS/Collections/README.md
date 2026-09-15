# Collections

The `Collections/` directory allows you to define custom game collections — curated lists of games that span multiple systems.

## How It Works

Each `.txt` file in this directory represents one collection. The file contains relative paths (relative to `/mnt/SDCARD/`) to ROM files, one per line.

## Example Collection File

```
# favorites.txt
/Roms/Game Boy (GB)/Metroid II.gb
/Roms/Super Nintendo Entertainment System (SUPA)/Super Metroid (World).smc
/Roms/Sony PlayStation (PS)/Castlevania - Symphony of the Night.m3u
```

## Creating a Collection

1. Create a `.txt` file in `Collections/` (e.g., `My Favorites.txt`).
2. Add ROM paths, one per line.
3. The collection will appear as a folder in the MinUI launcher root.

## Multi-Disc Games

For multi-disc games using `.m3u` playlists, you have two valid options in the collection file:

**Option A: Reference the `.m3u` file directly (recommended):**
```
/Roms/Sony PlayStation (PS)/Game Name/Game Name.m3u
```

**Option B: Reference the game folder:**
```
/Roms/Sony PlayStation (PS)/Game Name
```

Both methods correctly launch the game. The `.m3u` file path is more explicit and recommended. When launched, `minarch` reads the `.m3u` playlist and loads the first disc automatically.

## Notes

- Paths must start with `/` (absolute from SD card root).
- Entries are matched by file existence; use the `.m3u` path or folder path for multi-disc games.
- Collection entries can reference ROMs from any system folder.
- Blank lines are skipped.
- The `.txt` extension is stripped from the collection display name.
