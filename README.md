# MyMinUI

MyMinUI is a fork of the latest MinUI, I like MinUI but I also like playing old arcade coin up (thanks to FinUI) and DOOM which were missing so I added them.
A missing feature when using MinUI with arcades is that while in the official MinUI's cores usually the name of the rom is enough to identify a game with arcade the rom naming is quit difficult to decode so boxarts are nearly mandatory to properly identifying a game, that's why I spent a lot of time adding boxarts.
I'm a player that uses a lot savestates, I really can't understand why almost all firmwares available in the retrogaming do not provide a way to select a specific slot with a graphical preview, when I saw for the first time the in game menu of MinUI I immediately felt that that was the way, then I added the minarch code to minui.

You can find the latest release here: https://github.com/Turro75/MyMinUI/releases

## Supported device:

- m21:
  - SJGAM M21
  - SJGAM M22 PRO

## Docs:

- [Install (SJGAM M22 PRO)](docs/INSTALL.md)
- [Boxart](docs/BOXART.md)
- [Cheats](docs/CHEATS.md)
- [Multidisc](docs/MULTIDISC.md)
- [Paks](docs/PAKS.md)

## Features from FinUI:
- Add Favorites Collections (Press SELECT to toggle a rom)
- Clear "Recently Played"
- Additional emulators (MAME2003-PLUS)
- Base and Extras are merged into one Full release

## Main features added by MyMinUI to Min/FinUI till Today:
  - completely rewritten CPU rendering engine based on neon, multicores and double buffering to maximize performances avoiding screen tearing effect
  - additional aspect ratio options: extended (middle way between aspect and fullscreen), force 4:3 and 3:2 useful for squared screens.
  - provide MAX overclock setting
  - png boxart supported for systems and roms
  - added Fancy Mode to show boxart and selecting save states using previews.
  - integrated tool to convert existing boxart files to another size applying a custom blend level
  - integrated tool to make a live boxart within the in game menu
  - improved multidisc roms support (added pbp multidisc files)
  - cheats support (taken from NextUI)
  - extended debug HUD reporting fps rendered and generated, cpuload (per core), cpufreq, cpu temperature, battery level, source img size and current resolution and orientation
  - auto rotate games according to rom
  - ability to map left analog stick as dpad and ABXY buttons as right analog stick
  - play as player X feature that allows impersonating p2,p3 and p4.
  - auto detect 15,16 and 32bits source frame img.
  - implement audio fix (taken from NextUI) for better audio handling on some emulators.
  - reworked install/update process to let the user simply copying the release file to the sdcard
  - moved from libSDL to libSDL2
  - add 256MB of swapfile allowing neogeo games able to run.
  - added several emulators:
    - Prboom
    - native Pico8 support (needs purchased pico8 rpi binary)
    - Final Burn Neo
    - NeoGeo (Final Burn Neo)
    - NeoGeoCD
    - TyrQuake
    - Wonderswan Color
    - Atari 2600/5200/7800
    - Dosbox libretro
    - Amiga (not very good)


# MinUI

MinUI is a focused, custom launcher and libretro frontend for the RGB30, M17 (early revs), Trimui Smart (and Pro), Miyoo Mini (and Plus), and Anbernic RG35XX (and Plus).

<img src="github/minui-main.png" width=320 /> <img src="github/minui-menu-gbc.png" width=320 />
See [more screenshots](github/).

## Features

- Simple launcher, simple SD card
- No settings or configuration
- No boxart, themes, or distractions
- Automatically hides hidden files
  and extension and region/version
  cruft in display names
- Consistent in-emulator menu with
  quick access to save states, disc
  changing, and emulator options
- Automatically sleeps after 30 seconds
  or press POWER to sleep (and wake)
- Automatically powers off while asleep
  after two minutes or hold POWER for
  one second
- Automatically resumes right where
  you left off if powered off while
  in-game, manually or while asleep
- Resume from manually created, last
  used save state by pressing X in
  the launcher instead of A
- Streamlined emulator frontend
  (minarch + libretro cores)
- Single SD card compatible with
  multiple devices from different
  manufacturers

You can [grab the latest version here](https://github.com/shauninman/MinUI/releases).

> Devices with a physical power switch
> use MENU to sleep and wake instead of
> POWER. Once asleep the device can safely
> be powered off manually with the switch.

## Supported consoles

Base:

- Game Boy
- Game Boy Color
- Game Boy Advance
- Nintendo Entertainment System
- Super Nintendo Entertainment System
- Sega Genesis
- PlayStation

Extras:

- Neo Geo Pocket (and Color)
- Pico-8
- Pokémon mini
- Sega Game Gear
- Sega Master System
- Super Game Boy
- TurboGrafx-16 (and TurboGrafx-CD)
- Virtual Boy


## Legacy versions

The original Trimui Model S version of MinUI has been archived [here](https://github.com/shauninman/MinUI-Legacy-Trimui-Model-S).

The sequel, MiniUI for the Miyoo Mini, has been archived [here](https://github.com/shauninman/MiniUI-Legacy-Miyoo-Mini).

The return of MinUI for the Anbernic RG35XX has been archived [here](https://github.com/shauninman/MinUI-Legacy-RG35XX).
