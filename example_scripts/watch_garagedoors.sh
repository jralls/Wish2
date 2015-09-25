#!/bin/sh
#
# This script just runs as a shell process to watch for the on and off signal
# from an outdoor photo cell transmitter.  The one I use is a Leviton 6308.
#
# The transmitter is set to transmit on E16.  When it gets dark the transmitter
# will send E16 ON.  When it gets light the transmitter sends E16 OFF.  

dir="`dirname $0`/"

. ${dir}definitions.sh

old_status1="   "
old_status2="   "


while [ 1 = 1 ]
do
	status1=`nbread $doublegaragedoor`
	status2=`nbread $singlegaragedoor`
	if [ "$status1" != "$old_status1" -o "$status2" != "$old_status2" ]
	then
		old_status1="$status1"
		old_status2="$status2"
		if [ "$status1" = "000" -a "$status2" = "000" ]
		then
#			echo "Garage doors closed"
			echo 0 > $livingroom
			echo 0 > $livingroom
		else
#			echo "Garage doors open"
			echo 1 > $livingroom
			echo 1 > $livingroom
		fi
	fi
	sleep 1
done
