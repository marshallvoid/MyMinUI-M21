# Imgs

The `Imgs/` directory contains top-level images used by the MinUI launcher interface.

## Structure

```
EXTRAS/Imgs/
├── default.png              # Default boxart fallback for any game/system
├── HiddenRoms.png           # Icon for the Hidden ROMs folder
├── Collections.png          # Icon for the Collections folder
├── Recently Played.png      # Icon for the Recently Played folder
├── Favorites.png            # Icon for the Favorites folder
└── Tools.png                # Icon for the Tools (Options menu) section
```

## Boxart Fallback Chain

When no specific boxart is found for a game, MinUI searches:

1. `<ROM_DIR>/Imgs/<game_name>.png` — game-specific boxart
2. `<ROM_DIR>/Imgs/default.png` — system-level default
3. `Imgs/<folder_or_system_name>.png` — folder name boxart
4. `Imgs/default.png` — global default (this directory)
