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

/usr/local/etc/x10/action_alloff.sh
