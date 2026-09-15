# SYSTEM

The `SYSTEM/` directory contains MyMinUI internal system files — installed to `.system/` on the SD card. **Users typically do not need to modify** this directory.

## Structure

```
SYSTEM/
├── res/                # Shared resources (fonts, sprites, backgrounds)
│   ├── assets@1x.png   # Sprite sheet of icons (low-res)
│   ├── assets@2x.png
│   ├── assets@3x.png
│   ├── assets@4x.png
│   ├── BPreplayBold-unhinted.otf  # Menu font
│   ├── charging-640-480.png       # Charging screen image
│   └── ...
├── m21/                # Platform-specific files for SJGAM M21/M22 PRO
│   ├── _custombuttonmapping.env  # User button mapping overrides
│   ├── system.cfg      # System configuration (default limits)
│   ├── bin/            # System binaries
│   │   ├── minui.elf   # Main launcher
│   │   ├── minarch.elf # Libretro frontend
│   │   ├── keymon.elf  # Key monitor daemon
│   │   ├── led.sh      # LED control script
│   │   └── shutdown    # Shutdown script
│   ├── cores/          # Compiled libretro cores (.so)
│   ├── dat/            # System data (created at runtime)
│   ├── lib/            # Shared libraries (libmsettings.so, etc.)
│   └── paks/
│       └── MinUI.pak/  # MinUI startup package
│           └── launch.sh
```

## System Config (`system.cfg`)

```
-minarch_screen_sharpness = Crisp     # Screen sharpness setting
-minarch_thread_video = On            # Threaded video for cores
minarch_sync_reference = Auto         # Frame timing reference
minarch_prevent_tearing = lenient     # Vsync/tearing prevention
minarch_cpu_speed = Max               # Default CPU speed
minarch_screen_effect = None          # Screen effect (Line, Grid, None)
```

Lines prefixed with `-` are **locked** — the user cannot change them from the in-game menu.

## Custom Button Mapping (`_custombuttonmapping.env`)

Override default button mappings by uncommenting and changing the `USER_BTN_*` variables:

```bash
# Swap A and B buttons system-wide
export USER_BTN_A=289
export USER_BTN_B=290
```

## `bin/launch_rom.sh`

The ROM launcher script invoked by each emulator `.pak`. Parameters:
- `$1` — ROM file path
- `$2` — save state slot number
- `$3` — save state file path
- `$4` — 1 if SELECT is held (use RUN2 instead of RUN)
- `${5}` / `${6}` — RUN and RUN2 frontend names (`minarch`)
- `${7}` — path to the core `.pak` directory
- `${8}` — core executable name (e.g., `pcsx_rearmed`)
- `${CPU_OC}` — desired CPU frequency
