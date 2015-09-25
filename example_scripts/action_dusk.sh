#!/bin/sh
#

ver="`dirname $0`/"
. ${ver}definitions.sh

if [ -f /etc/init.d/functions ]; then
	. /etc/init.d/functions
elif [ -f /etc/rc.d/init.d/functions ]; then
	. /etc/rc.d/init.d/functions
else
	exit 0
fi

/usr/local/etc/x10/action_night.sh on
/usr/bin/x10watch /dev/x10/g11 -0 "/usr/local/etc/x10/action_garage.sh close" -1 "/usr/local/etc/x10/action_garage.sh open" -p 300 & 
/usr/bin/x10watch /dev/x10/g12 -0 "/usr/local/etc/x10/action_garage.sh close" -1 "/usr/local/etc/x10/action_garage.sh open" -p 300 &
