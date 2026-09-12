# MinUI

# NOTE: this runs on the host system (eg. macOS) not in a docker image
# it has to, otherwise we'd be running a docker in a docker and oof

ifeq (,$(PLATFORMS))
PLATFORMS = m21
endif

###########################################################

BUILD_HASH:=$(shell git rev-parse --short HEAD)
RELEASE_TIME:=$(shell TZ=GMT date +%Y%m%d)
RELEASE_BETA=b
RELEASE_BASE=MyMinUI-$(RELEASE_TIME)$(RELEASE_BETA)
RELEASE_DOT:=$(shell find ./releases/. | grep -e ".*/${RELEASE_BASE}-[0-9]+-base\.zip" | wc -l | sed 's/ //g')

RELEASE_NAME=$(RELEASE_BASE)-$(RELEASE_DOT)


###########################################################
.PHONY: build

export MAKEFLAGS=--no-print-directory

name:
	echo $(RELEASE_NAME)

shell:
	make -f makefile.toolchain PLATFORM=$(PLATFORM)

clean:
	rm -rf ./build
	rm -rf ./releases

setup:
	# ----------------------------------------------------
	# make sure we're running in an input device
	tty -s

	# ready fresh build
	rm -rf ./build
	mkdir -p ./releases
	mkdir -p ./build/SYSTEM
	mkdir -p ./build/EXTRAS/Emus
	mkdir -p ./build/EXTRAS/Tools
	mkdir -p ./build/PAYLOAD

	cp -R ./skeleton/BASE ./build/BASE
	cp -R ./skeleton/SYSTEM/res ./build/SYSTEM/res
	cp -R ./skeleton/SYSTEM/$(PLATFORM) ./build/SYSTEM/$(PLATFORM)
	cp -R ./skeleton/EXTRAS/Bios ./build/EXTRAS/Bios
	cp -R ./skeleton/EXTRAS/Roms ./build/EXTRAS/Roms
	cp -R ./skeleton/EXTRAS/Cheats ./build/EXTRAS/Cheats
	cp -R ./skeleton/EXTRAS/Collections ./build/EXTRAS/Collections
	cp -R ./skeleton/EXTRAS/Emus/$(PLATFORM) ./build/EXTRAS/Emus/$(PLATFORM)
	cp -R ./skeleton/EXTRAS/Imgs ./build/EXTRAS/Imgs
	cp -R ./skeleton/EXTRAS/Tools/$(PLATFORM) ./build/EXTRAS/Tools/$(PLATFORM)

	# remove authoring detritus
	cd ./build && find . -type f -name '.keep' -delete
	cd ./build && find . -type f -name '*.meta' -delete
	echo $(BUILD_HASH) > ./workspace/hash.txt

	# copy readmes to workspace so we can use Linux fmt instead of host's
	mkdir -p ./workspace/readmes
	cp ./skeleton/BASE/README.txt ./workspace/readmes/BASE-in.txt
# 	cp ./skeleton/EXTRAS/README.txt ./workspace/readmes/EXTRAS-in.txt

common: build system cores

build:
	# ----------------------------------------------------
	make build -f makefile.toolchain PLATFORM=$(PLATFORM)
	# ----------------------------------------------------

system:
	make -f ./workspace/$(PLATFORM)/platform/makefile.copy PLATFORM=$(PLATFORM)

	# populate system
	cp ./workspace/$(PLATFORM)/keymon/keymon.elf ./build/SYSTEM/$(PLATFORM)/bin/
	cp ./workspace/$(PLATFORM)/libmsettings/libmsettings.so ./build/SYSTEM/$(PLATFORM)/lib
	cp ./workspace/all/minui/build/$(PLATFORM)/minui.elf ./build/SYSTEM/$(PLATFORM)/bin/
	cp ./workspace/all/minarch/build/$(PLATFORM)/minarch.elf ./build/SYSTEM/$(PLATFORM)/bin/
	cp ./workspace/all/clock/build/$(PLATFORM)/clock.elf ./build/EXTRAS/Tools/$(PLATFORM)/Clock.pak/
	cp ./workspace/all/minput/build/$(PLATFORM)/minput.elf ./build/EXTRAS/Tools/$(PLATFORM)/Input.pak/
	cp ./workspace/all/clear_recent/build/$(PLATFORM)/clear_recent.elf "./build/EXTRAS/Tools/$(PLATFORM)/Clear Recently Played.pak/"
	cp ./workspace/all/convertboxart/build/$(PLATFORM)/convertboxart.elf "./build/EXTRAS/Tools/$(PLATFORM)/Convert BoxArt.pak/"
	cp ./workspace/all/Commander/build/$(PLATFORM)/MyCommander.elf "./build/EXTRAS/Tools/$(PLATFORM)/Files.pak/"
	cp -r ./workspace/all/Commander/res "./build/EXTRAS/Tools/$(PLATFORM)/Files.pak/"

cores:
	cp ./workspace/$(PLATFORM)/cores/output/* ./build/SYSTEM/$(PLATFORM)/cores/
#   mv ./build/SYSTEM/$(PLATFORM)/cores/retroarch ./build/SYSTEM/$(PLATFORM)/bin/retroarch.elf
# 	mv ./build/SYSTEM/$(PLATFORM)/cores/bmp2png ./build/SYSTEM/$(PLATFORM)/bin/bmp2png.elf

package:
	# ----------------------------------------------------
	# zip up build

	# move formatted readmes from workspace to build
	cp ./workspace/readmes/BASE-out.txt ./build/BASE/README.txt
	# cp ./workspace/readmes/EXTRAS-out.txt ./build/EXTRAS/README_EXTRAS.txt
	rm -rf ./workspace/readmes

	cd ./build/SYSTEM && echo "$(RELEASE_NAME)\n$(BUILD_HASH)" > version.txt
	./commits.sh > ./build/SYSTEM/commits.txt
	cd ./build && find . -type f -name '.DS_Store' -delete

	mv ./build/SYSTEM ./build/PAYLOAD/.system


	cd ./build/PAYLOAD && zip -r MinUI.zip .system
	mv ./build/PAYLOAD/MinUI.zip ./build/BASE


	rm -fr ./build/FULL
	mkdir ./build/FULL
	cp -fR ./build/BASE/* ./build/FULL/
	cp -fR ./build/EXTRAS/* ./build/FULL/
	rm -rf ./build/BASE
	rm -rf ./build/EXTRAS
	rm -rf ./build/PAYLOAD
	rm -rf ./releases/$(RELEASE_NAME)-$(PLATFORM).zip
	cd ./build/FULL && zip -r ../../releases/$(RELEASE_NAME)-$(PLATFORM).zip Bios Cheats Collections Emus Imgs Roms Tools m21 MinUI.zip README.txt


	echo "$(RELEASE_NAME)" > ./build/latest.txt
	make done

done:
	command -v say >/dev/null 2>&1 && say "done" || echo "done"


###########################################################

all: setup $(PLATFORMS) done

m21:
	# ----------------------------------------------------
	make clean setup common package PLATFORM=$@
	# ----------------------------------------------------
