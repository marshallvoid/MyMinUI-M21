#!/bin/sh
# LED control for SJGAM M22 PRO (m21) - MinUI
# Modes: off | gradient (breathing cyan wave, Stock-like)
# Setting persists in LED_CONF so reboot restores the user's choice.
#
# Runs both outside chroot (boot.sh) and inside chroot (MinUI.pak,
# Tools paks). /tmp is bind-mounted so the pidfile is shared.

SDCARD="${SDCARD_PATH:-/mnt/SDCARD}"
LED_CONF="$SDCARD/.userdata/m21/led.conf"
PIDFILE="/tmp/led.pid"

LEDS=16
MAXB=255
DEFAULT_LED_BRIGHT=60

has_leds() {
	[ -e /sys/class/leds/sunxi_led0r/brightness ]
}

# Read brightness from led.conf (line 2), default to DEFAULT_LED_BRIGHT
read_brightness() {
	if [ -f "$LED_CONF" ]; then
		BRIGHT=$(sed -n '2p' "$LED_CONF" 2>/dev/null | tr -d ' \t\r\n')
		if [ -n "$BRIGHT" ] && [ "$BRIGHT" -ge 0 ] 2>/dev/null && [ "$BRIGHT" -le 255 ] 2>/dev/null; then
			echo "$BRIGHT"
			return
		fi
	fi
	echo "$DEFAULT_LED_BRIGHT"
}

# Write mode and brightness to led.conf (atomic via temp file)
write_conf() {
	mkdir -p "$(dirname "$LED_CONF")"
	tmp="$LED_CONF.tmp.$$"
	printf "%s\n%s\n" "$1" "$2" > "$tmp" 2>/dev/null
	mv "$tmp" "$LED_CONF" 2>/dev/null
}

write_all() {
	# $1=R $2=G $3=B
	# take the LEDs out of any kernel trigger first, otherwise the
	# kernel owns brightness and our writes get overridden
	i=0
	while [ $i -lt $LEDS ]; do
		echo none > /sys/class/leds/sunxi_led${i}r/trigger 2>/dev/null
		echo none > /sys/class/leds/sunxi_led${i}g/trigger 2>/dev/null
		echo none > /sys/class/leds/sunxi_led${i}b/trigger 2>/dev/null
		echo "$1" > /sys/class/leds/sunxi_led${i}r/brightness 2>/dev/null
		echo "$2" > /sys/class/leds/sunxi_led${i}g/brightness 2>/dev/null
		echo "$3" > /sys/class/leds/sunxi_led${i}b/brightness 2>/dev/null
		i=$((i + 1))
	done
}

led_stop() {
	if [ -f "$PIDFILE" ]; then
		PID=$(cat "$PIDFILE" 2>/dev/null)
		if [ -n "$PID" ]; then
			kill "$PID" 2>/dev/null
			# Brief wait to ensure the daemon exits and stops
			# writing gradient frames that would override led_off
			sleep 0.1 2>/dev/null || sleep 1
		fi
		rm -f "$PIDFILE"
	fi
}

led_off() {
	led_stop
	write_all 0 0 0
}

gradient_daemon() {
	# Stock-like rainbow wave: hue rotates over time and each LED is
	# phase-shifted around the ring (integer HSV->RGB, no floats)
	trap "exit 0" TERM INT
	t=0
	brightness=$(read_brightness)
	brightness_check=0
	while true; do
		# Refresh brightness from config periodically (every ~60 frames)
		brightness_check=$((brightness_check + 1))
		if [ $brightness_check -ge 60 ]; then
			brightness_check=0
			new_brightness=$(read_brightness)
			if [ "$new_brightness" != "$brightness" ]; then
				brightness=$new_brightness
			fi
		fi
		i=0
		while [ $i -lt $LEDS ]; do
			hue=$(( (t + i * 360 / LEDS) % 360 ))
			val=$brightness
			# HSV(hue,100%,val) -> RGB, hue sector 0..5
			sector=$(( hue / 60 ))
			frac=$(( hue % 60 ))
			p=0
			q=$(( val * (60 - frac) / 60 ))
			tt=$(( val * frac / 60 ))
			case $sector in
				0) rr=$val; gg=$tt; bb=0 ;;
				1) rr=$q; gg=$val; bb=0 ;;
				2) rr=0; gg=$val; bb=$tt ;;
				3) rr=0; gg=$q; bb=$val ;;
				4) rr=$tt; gg=0; bb=$val ;;
				*) rr=$val; gg=0; bb=$q ;;
			esac
			rr=$(( rr * MAXB / 255 )); gg=$(( gg * MAXB / 255 )); bb=$(( bb * MAXB / 255 ))
			echo $rr > /sys/class/leds/sunxi_led${i}r/brightness 2>/dev/null
			echo $gg > /sys/class/leds/sunxi_led${i}g/brightness 2>/dev/null
			echo $bb > /sys/class/leds/sunxi_led${i}b/brightness 2>/dev/null
			i=$((i + 1))
		done
		t=$(( (t + 3) % 360 ))
		sleep 0.08
	done
}

led_gradient() {
	led_stop
	# NOTE: no subshell here -- $! must be the daemon PID so led_stop()
	# can kill it later (a subshell would record a dead parent PID)
	gradient_daemon >/dev/null 2>&1 &
	echo $! > "$PIDFILE"
}

read_mode() {
	if [ -f "$LED_CONF" ]; then
		MODE=$(head -1 "$LED_CONF" 2>/dev/null | tr -d ' \t\r\n')
	else
		MODE="gradient"
	fi
	case "$MODE" in
		off|gradient) ;;
		*) MODE="gradient" ;;
	esac
	echo "$MODE"
}

cmd="$1"
BRIGHTNESS=$(read_brightness)
case "$cmd" in
	off)
		has_leds || exit 0
		write_conf "off" "$BRIGHTNESS"
		led_off
		;;
	off-now)
		# poweroff path: kill the daemon and cut the LEDs WITHOUT
		# touching led.conf, so the next boot still restores the
		# user's saved mode
		has_leds || exit 0
		led_off
		;;
	gradient)
		has_leds || exit 0
		write_conf "gradient" "$BRIGHTNESS"
		led_gradient
		;;
	toggle)
		has_leds || exit 0
		if [ "$(read_mode)" = "gradient" ]; then
			write_conf "off" "$BRIGHTNESS"
			led_off
		else
			write_conf "gradient" "$BRIGHTNESS"
			led_gradient
		fi
		;;
	status)
		read_mode
		;;
	apply|""|*)
		has_leds || exit 0
		if [ "$(read_mode)" = "off" ]; then
			led_off
		else
			led_gradient
		fi
		;;
esac
exit 0
