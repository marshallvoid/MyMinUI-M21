## MyMinUI install instructions

### SJGAM M21 / M22 PRO

Format as FAT32 (exFAT is also supported) a micro SD card with enough space to hold all your games. The volume name doesn't matter.

In the release zip (e.g. `MyMinUI-YYYYMMDDb-0-m21.zip`), copy the `m21` folder to the root of the FAT32 partition.

Move (or copy) `m21/emulationstation` and `m21/tomato` to the root of the FAT32 partition as well.

Copy the whole release zip file (leave it zipped) to the root of the FAT32 partition.

The SD card file structure must be:

```
├── MyMinUI-YYYYMMDDb-0-m21.zip
├── emulationstation
├── tomato
└── m21
    ├── updating.png
    ├── installing.png
    ├── emulationstation
    ├── tomato
    ├── libmusl
    │   ├── libSDL2-2.0.so.0
    │   ├── libasound.so.2
    │   ├── libext2fs.so.2
    │   ├── libblkid.so.1
    │   ├── libSDL-1.2.so.0
    │   ├── libpng16.so.16
    │   ├── libfuse.so.2
    │   ├── libmsettings.so
    │   ├── libz.so.1
    │   ├── libSDL2_image-2.0.so.0
    │   ├── libSDL2_ttf-2.0.so.0
    │   ├── libSDL_ttf-1.2.so.0
    │   ├── libSDL_image-1.2.so.0
    │   └── libcom_err.so.2
    └── binmusl
        ├── unzip
        ├── amixer
        ├── fuse2fs
        └── show.elf
```

Put the SD card in the device and boot. A screen will report "Installing MyMinUI..." then, after a while (be patient — swap creation takes time). Once installation is complete, switch off the POWER switch to shut down the device, remove the SD card, and insert it into your PC to fill the BIOS and ROMs folders.

---

After installation, see the [Usage Guide](USAGE.md) for shortcuts and features, and the topic-specific guides for [Boxart](BOXART.md), [Cheats](CHEATS.md), [Multidisc](MULTIDISC.md), and [Paks](PAKS.md).
