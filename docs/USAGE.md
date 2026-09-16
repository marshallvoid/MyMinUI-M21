# MyMinUI Usage Guide

## Shortcuts

### SJGAM M21 / M22 PRO

- **Brightness**: SELECT + VOLUME UP / DOWN
- **LED Brightness**: START + VOLUME UP / DOWN
- **Volume**: VOLUME UP / DOWN (no modifier)

### Sleep, Wake, and Power Off

#### M21 / M22 PRO

- **Sleep**: 2-minute inactivity timeout
- **Wake**: MENU button
- **Power Off**: Hold MENU for more than 2 seconds, then switch off the POWER switch when the screen goes dark

#### M22 PRO (no MENU button)

- **Menu**: Hold SELECT + START

#### In-game

- **In-game menu**: MENU button (M22 PRO: SELECT + START)

## Auto Sleep and Power Off Delays

MyMinUI activates sleep mode after 30 seconds of inactivity while in menus (main and game). Power-off occurs after 2 minutes of sleep.

These delays are adjustable by editing text files:

- `.userdata/shared/sleep-delay-sec` — auto-sleep delay (0 disables)
- `.userdata/shared/poweroff-delay-sec` — auto-power-off after sleep delay (0 disables)

Both values are in seconds and limited to 7200 (2 hours).

## Working Modes

MyMinUI has 3 working modes: **Standard**, **Simple**, and **Fancy**.

To switch modes: press MENU, then use UP/DOWN to change.

### Standard Mode (Default)

Same look and feel of MinUI, keeping additional MyMinUI features.

### Fancy Mode

A reworked layout that shows boxart on the right side and allows selecting save states using previews. See the [Boxart Guide](BOXART.md) for dimensions and locations.

### Simple Mode

Hides the Tools folder and replaces the Options menu in the in-game menu with Reset. Perfect for handing off to children.

To enable Simple Mode, create an empty file named `enable-simple-mode` (no extension) in `/.userdata/shared/`.

## Favorites

**Press SELECT to toggle a ROM as a favorite.**

## Quicksave & Auto-Resume

MyMinUI automatically creates a quicksave when powering off in-game. The next time you power on the device, it will automatically resume from where you left off.

A quicksave is created when powering off manually, automatically after a short sleep, or when the device powers off while asleep.

On the M21, press the MENU button twice to put the device to sleep before flipping the POWER switch.

## Roms

The SD card includes a `Roms` folder with subfolders for each supported console. You can rename these folders but must keep the uppercase tag name in parentheses to retain the correct emulator mapping. For example, `Nintendo Entertainment System (FC)` can be renamed to `Nintendo (FC)`, `NES (FC)`, or `Famicom (FC)`.

When two or more folders share the same display name (e.g., `Game Boy Advance (GBA)` and `Game Boy Advance (MGBA)`), they are combined into a single menu item containing ROMs from both folders. This allows opening specific ROMs with an alternate pak.

## Supported Consoles

### Base

- Game Boy
- Game Boy Color
- Game Boy Advance
- Nintendo Entertainment System
- Super Nintendo Entertainment System
- Sega Genesis
- PlayStation

### Extras

- Neo Geo Pocket (and Color)
- Pico-8
- Pokémon Mini
- Sega Game Gear
- Sega Master System
- Super Game Boy
- TurboGrafx-16 (and TurboGrafx-CD)
- Virtual Boy
- Final Burn Neo (arcade)
- NeoGeo (Final Burn Neo)
- NeoGeoCD
- MAME2003-PLUS
- Doom
- TyrQuake
- Wonderswan / Wonderswan Color
- Atari 2600 / 5200 / 7800
- DOS (DOSBox)
- Amiga

## Non-US Font Support

If you need non-US fonts, any OTF/TTF font that supports your charset can replace the system fonts:

- `.system/m21/res/BPreplayBold-unhinted.otf` (for MinUI, MinArch, and tools)
- `Tools/m21/Files/Res/FreeSans.ttf` (specific to the Files tool)

Font files will be overwritten on updates.

---

For more details, see the [Installation Guide](INSTALL.md) and the topic-specific guides: [Boxart](BOXART.md), [Cheats](CHEATS.md), [Multidisc](MULTIDISC.md), [Paks](PAKS.md).
