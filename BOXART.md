# Command line on PC to convert any image to a good boxart for MyMinUI
needs ImageMagick

this is infile.bmp:

![MyMinUI](./github/gamename.png)

magick infile.bmp -resize 640x480! \( +clone -fx 'i/w < 0.2 ? 0.1 : (i/w - 0.2)*2 +0.1' \) -compose multiply -composite outfile.png

this is the resulting outfile.png ready to copied in the Imgs folder:

![MyMinUI](./github/boxart.png)

The example Infile.bmp is actually a save state preview taken direclty from the device, state previews are stored in /mnt/sdcard/.userdata/shared/.minui/<EMU_FOLDER>/\<gamename\>.\<ext\>.\<slot\>.bmp

# MyMinUI add 2 more ways to get a boxart/game preview directly on the device:

In menu game select "make Boxart" to create a boxart of the current game screen
In "Tools/Convert Boxart" You can batch converting the images (png and bmp format only) from a directory with a specific format.
Currently the source directory is fixed in code, just copy the images You want to convert in the folder "/mnt/sdcard/Tools/${PLATFORM}/Convert Boxart.pak/srcimgdir" all converted images are stored in
"/mnt/sdcard/Tools/${PLATFORM}/Convert Boxart.pak/srcimgdir/outdir", the outdir is auto created if not existing.

there are 2 different config files to set the boxart result.
/mnt/sdcard/Tools/Convert Boxart.pak/gameboxart.cfg used by minarch (in game menu)
/mnt/sdcard/Tools/Convert Boxart.pak/toolboxart.cfg used by convertboxart (convert boxart tool)

both have the same parameters:

    //screen pixels size
    SW = 640
    SH = 480

SW and SH define the specific device resolution, for most devices is 640x480

    //BX, BY, BW, BH define the target box where to place the image, BX and BY define the position of the top left corner of the box, BW and BH define the width and the height of the target box
    BX = 160
    BY = 60
    BW = 480
    BH = 360

these values define the target box position and size where the image (or the screenshot) will be fitted, looking at the screen the top left corner is BX=0,BY=0 while the bottom rigth corner is BX=640,BY=480.

    //the aspect may be 0 which means ASPECT which preserves the original aspect ratio of the e image (or the screenshot) by filling the target box
    //ASPECT = 0
    //the aspect may be 1 which means NATIVE which preserves the original image (or the screenshot) size and aspect ratio placing it in the middle of the target box,
        if the original size is bigger than target box then ASPECT rule is applied
    //ASPECT = 1
    //the aspect may be 2 which means FULL which resizes the image (or the screenshot) to full fit the target box ignoring original aspect ratio
    ASPECT = 2

and last it can be defined a gradient file to add transparency effect to the resulting image

    //the gradient is a png file with transparency set which is applied to the image, define a full path file name or NONE to skip this step
    GRADIENT = /mnt/sdcard/Tools/rg35xx/Convert BoxArt.pak/BlackGradient.png
    //GRADIENT = NONE

The provided toolboxart.cfg file is optimized for batch conversion of images from Garlic OS themes
The provided gameboxart.cfg file is optimized for optimal filling of a game preview with fancy mode in MyMinUI.

# Boxart locations (Fancy mode)

- games and collated/multi-disc folders: `/Roms/<EMU>/Imgs/<game name>.png` (as before)
- console folders: `/Roms/<EMU>/Imgs/<EMU folder name>.png`
- collections (each .txt inside /Collections): `/Collections/Imgs/<collection name>.png`
- system folders like Recently Played, Favorites, HiddenRoms, Collections, Tools: `/Imgs/<folder name>.png` (e.g. `/Imgs/Favorites.png`)

Boxart images are decoded and scaled once, cached, and preloaded for the whole
current folder while the menu is idle, so browsing stays smooth.
