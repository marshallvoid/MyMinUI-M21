#!/bin/sh

########################################


export PLATFORM="m21"
export SDCARD_PATH="/mnt/SDCARD"
export BIOS_PATH="$SDCARD_PATH/Bios"
export SAVES_PATH="$SDCARD_PATH/Saves"
export CHEATS_PATH="$SDCARD_PATH/Cheats"
export SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"
export CORES_PATH="$SYSTEM_PATH/cores"
export USERDATA_PATH="$SDCARD_PATH/.userdata/$PLATFORM"
export SHARED_USERDATA_PATH="$SDCARD_PATH/.userdata/shared"
export LOGS_PATH="$USERDATA_PATH/logs"
export DATETIME_PATH="$SHARED_USERDATA_PATH/datetime.txt"

mkdir -p "$USERDATA_PATH"
mkdir -p "$CHEATS_PATH"
mkdir -p "$LOGS_PATH"
mkdir -p "$SHARED_USERDATA_PATH/.minui"

#check if newdtb is loaded
TMPSTR=$(cat /sys/class/disp/disp/attr/sys | grep fps:38)
if [ "${TMPSTR}NULL" = "NULL" ]; then
	export NEWDTB=1
fi

echo 1 > /sys/class/disp/disp/attr/colorbar
export PATH=$SYSTEM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:$LD_LIBRARY_PATH
export SDL_NOMOUSE=1

source "$SYSTEM_PATH/custombuttonmapping.env"

#######################################

# LED sysfs dump for debugging (M22 PRO)
#{
#	echo "date: $(date)"
#	echo "--- ls -l /sys/class/leds:"
#	ls -l /sys/class/leds/
#	for d in /sys/class/leds/*; do
#		echo "== $d =="
#		for f in trigger max_brightness brightness device/modalias device/name uevent; do
#			if [ -e "$d/$f" ]; then
#				echo "--- $f:"
#				cat "$d/$f" 2>&1
#				echo
#			fi
#		done
#	done
#} > "$SDCARD_PATH/led-dump-minui.txt" 2>&1
#sync

# restore saved LED mode (off|gradient), persists across reboots
if [ -f "$SYSTEM_PATH/bin/led.sh" ]; then
	sh "$SYSTEM_PATH/bin/led.sh" apply >/dev/null 2>&1 &
fi


#Available frequency
#480000
#720000
#912000
#1008000
#1104000
#1200000

export CPU_SPEED_MENU=912000
export CPU_SPEED_POWERSAVE=912000
export CPU_SPEED_GAME=1008000
export CPU_SPEED_PERF=1104000
export CPU_SPEED_MAX=1200000

export GOVERNOR_PATH="/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
export GOVERNOR_CPUSPEED_PATH="/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"
echo userspace > "${GOVERNOR_PATH}"
echo $CPU_SPEED_PERF > "${GOVERNOR_CPUSPEED_PATH}"

#######################################

keymon.elf & #> $LOGS_PATH/keymon.txt 2>&1 &

#######################################

# init datetime
if [ -f "$DATETIME_PATH" ]; then
	DATETIME=`cat "$DATETIME_PATH"`
	date +'%F %T' -s "$DATETIME"
	DATETIME=`date +'%s'`
	date -u -s "@$DATETIME"
	hwclock --utc -w
fi

#######################################

AUTO_PATH="$USERDATA_PATH/auto.sh"
if [ -f "$AUTO_PATH" ]; then
	"$AUTO_PATH" # > $LOGS_PATH/auto.txt 2>&1
fi

cd $(dirname "$0")

#######################################

EXEC_PATH="/tmp/minui_exec"
NEXT_PATH="/tmp/next"
touch "$EXEC_PATH" && sync
while [ -f "$EXEC_PATH" ]; do
	echo $CPU_SPEED_GAME > "${GOVERNOR_CPUSPEED_PATH}"
	minui.elf > $LOGS_PATH/minui.txt 2>&1

	echo `date +'%F %T'` > "$DATETIME_PATH"
	sync

	if [ -f $NEXT_PATH ]; then
		CMD=`cat $NEXT_PATH`
		eval $CMD
		rm -f $NEXT_PATH
		echo $CPU_SPEED_GAME > "${GOVERNOR_CPUSPEED_PATH}"
		echo `date +'%F %T'` > "$DATETIME_PATH"
		sync
	fi

	# physical powerswitch, enter low power mode
	if [ -f "/tmp/poweroff" ]; then
		rm -f "/tmp/poweroff"
		killall keymon.elf
		# cut the LEDs too (daemon would keep them lit until hard power cut)
		if [ -f "$SYSTEM_PATH/bin/led.sh" ]; then
			sh "$SYSTEM_PATH/bin/led.sh" off-now >/dev/null 2>&1
		fi
		shutdown
		while :; do
			sleep 5
		done
	fi
done
