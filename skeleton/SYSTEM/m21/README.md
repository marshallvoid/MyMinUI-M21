# SYSTEM/m21

Platform-specific files for the **SJGAM M21 / M22 PRO** handheld (Allwinner H133 SoC, 640x480 screen).

## Files

| File | Description |
|---|---|
| `system.cfg` | System-level configuration with locked defaults (lines prefixed with `-`) |
| `_custombuttonmapping.env` | Shell env file for overriding button mappings |
| `bin/launch_rom.sh` | ROM launcher script called by every emulator `.pak` |
| `bin/led.sh` | LED control script (off / gradient breathing modes) |
| `bin/shutdown` | Shutdown script (saves datetime, powers off) |
| `cores/` | Compiled libretro core shared libraries (`.so`) |
| `dat/` | Runtime data directory |
| `lib/` | Shared libraries (libmsettings, SDL2, etc.) |
| `paks/MinUI.pak/launch.sh` | Main MinUI launcher entry script |

## `system.cfg` Locked Options

Lines prefixed with `-` are locked — the user cannot override them from the in-game menu:

```
-minarch_screen_sharpness = Crisp     # Locked: always Crisp
-minarch_thread_video = On            # Locked: threaded video enabled
minarch_sync_reference = Auto         # User-modifiable
minarch_prevent_tearing = lenient     # User-modifiable
minarch_cpu_speed = Max               # Locked: default to Max overclock
minarch_screen_effect = None          # User-modifiable
```

## `launch_rom.sh` Parameters

```
$1  — ROM file path
$2  — save state slot number
$3  — save state file path
$4  — 1 if SELECT held (use RUN2 instead of RUN)
$5  — RUN frontend name (minarch)
$6  — RUN2 frontend name (minarch)
$7  — core .pak directory path
$8  — core executable name (e.g., pcsx_rearmed)
$CPU_OC — CPU frequency
```

## `bin/launch_rom.sh` Environment

Required environment variables (set by `MinUI.pak/launch.sh`):

```
PLATFORM     — "m21"
SDCARD_PATH    — "/mnt/SDCARD"
BIOS_PATH      — "$SDCARD_PATH/Bios"
SAVES_PATH     — "$SDCARD_PATH/Saves"
CHEATS_PATH    — "$SDCARD_PATH/Cheats"
SYSTEM_PATH    — "$SDCARD_PATH/.system/m21"
CORES_PATH     — system cores directory
USERDATA_PATH  — per-platform user data
SHARED_USERDATA_PATH — shared userdata across platforms
CPU_SPEED_*    — CPU frequency presets (MENU, POWERSAVE, GAME, PERF, MAX)
```

## Button Mapping

See `_custombuttonmapping.env` for the default button-to-keycode mapping. M22 PRO lacks a dedicated Menu button — it's emulated by pressing Select + Start simultaneously. Long-pressing Menu triggers power-off (after `PWR_TIMEOUT` = 2000ms).
