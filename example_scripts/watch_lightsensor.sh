#!/bin/sh
#
# This script just runs as a shell process to watch for the on and off signal
# from an outdoor photo cell transmitter.  The one I use is a Leviton 6308.
#
# The transmitter is set to transmit on E16.  When it gets dark the transmitter
# will send E16 ON.  When it gets light the transmitter sends E16 OFF.  

dir="`dirname $0`/"
. ${dir}definitions.sh

device=$photocell

old_status="   "


while [ 1 = 1 ]
do
	status=`nbread $device`
	if [ "$status" != "$old_status" ]
	then
		old_status="$status"
		if [ "$status" = "000" ]
		then
#			echo "PhotoCell:  Day"
			${dir}action_alloff.sh
		else
#			echo "PhotoCell:  Night"
			${dir}action_night.sh on
		fi
	fi
	sleep 10
done
