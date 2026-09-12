## MyMinUI install instructions:

# SJGAM M21/M22pro:

Format as FAT32 (it is also supported exFAT if You like) a micro sdcard with enough space to contains all Your games, the volume name assigned to the partition doesn't matter.
in the release file (i.e. MyMinUI-YYYYMMDDb-0-m21.zip) there is a folder called "m21" copy that folder as is in the FAT32 partition created above.
Move (or copy) the file m21/emulationstation and m21/tomato to the root of the FAT32 partition created above.
Copy also the whole release zip file (leave it zipped) in the FAT32 partition created above.

The sdcard file structure must be:
<pre>
├── MyMinUI-YYYYMMDDb-0-m21.zip
├── emulationstation
├── tomato
└── m21
    ├── updating.png
    ├── installing.png
    ├── emulationstation
    ├── tomato
    ├── libmusl
    │   ├── libSDL2-2.0.so.0
    │   ├── libasound.so.2
    │   ├── libext2fs.so.2
    │   ├── libblkid.so.1
    │   ├── libSDL-1.2.so.0
    │   ├── libpng16.so.16
    │   ├── libfuse.so.2
    │   ├── libmsettings.so
    │   ├── libz.so.1
    │   ├── libSDL2_image-2.0.so.0
    │   ├── libSDL2_ttf-2.0.so.0
    │   ├── libSDL_ttf-2.0.so.0
    │   ├── libSDL_image-1.2.so.0
    │   └── libcom_err.so.2
    └── binmusl
        ├── unzip
        ├── amixer
        ├── fuse2fs
        └── show.elf
</pre>

Put the sdcard in the device and boot, a screen reporting "Installing MyMinUI..." then after a while (be patient in this stage as the swap creation process takes time)
Once installation process is completed switch off the PWR switch to shutdown the device, remove the sdcard and insert it in the pc, now You can fill the bios and roms folders with Your files. Put the sdcard in the device and play Your games.
