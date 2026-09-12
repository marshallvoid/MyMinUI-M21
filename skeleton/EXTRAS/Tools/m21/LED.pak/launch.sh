#!/bin/sh
# Toggle LED: off <-> gradient (breathing wave).
# The choice is saved to .userdata/m21/led.conf and re-applied on boot.

SDCARD_PATH="/mnt/SDCARD"
PLATFORM="m21"
SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"

if [ -f "$SYSTEM_PATH/bin/led.sh" ]; then
	sh "$SYSTEM_PATH/bin/led.sh" toggle >/dev/null 2>&1
fi
