#!/bin/sh
#
# chkconfig: - 99 99
# description: Starts and stops the X10 control interface \
#

# Source function library.
if [ -f /etc/init.d/functions ] ; then
  . /etc/init.d/functions
elif [ -f /etc/rc.d/init.d/functions ] ; then
  . /etc/rc.d/init.d/functions
else
  exit 0
fi

# Avoid using root's TMPDIR
unset TMPDIR

# Source networking configuration.
. /etc/sysconfig/network

RETVAL=0

# Try to figure out the /dev device
rawdev=`dmesg | grep "^hiddev" | grep -m 1 "PowerLinc" | awk -F: '{print $1}'`
if [ "$rawdev" = "" ]; then
  echo "Error:  No PowerLinc USB device found"
  exit 1
fi
basedev=`dmesg | grep -m 1 "^hiddev" | awk -F: '{print $1}'`
basedev=`echo $basedev | sed 's/hiddev//g'`
hiddev=`echo $rawdev | sed 's/hiddev//g'`
hiddev=`expr $hiddev - $basedev`
hiddev="hiddev${hiddev}"
hiddev=`find /dev -name $hiddev -print`
echo "PowerLinc found on device $hiddev ($rawdev)"

start() {
	echo -n $"Starting X10 services: "
	if [ -f /var/log/x10log ]
	then
		rm -f /var/log/x10log
	fi
	modprobe x10
	/usr/sbin/plusbd -device $hiddev
	/usr/sbin/x10logd
	RETVAL=$?
	if [ -f /usr/local/etc/x10.local ]; then
		/usr/local/etc/x10.local start
	fi
	echo
	return $RETVAL
}	

stop() {
	echo -n $"Shutting down X10 services: "
	RETVAL=0
	if [ -f /usr/local/etc/x10.local ]; then
		/usr/local/etc/x10.local stop
	fi
	killproc x10logd
	killproc plusbd -HUP
	rmmod x10
	RETVAL=$?
	return $RETVAL
}	

restart() {
	stop
	start
}	

reload() {
        echo -n $"Reloading x10attach file: "
	killproc plusbd -HUP
	RETVAL=$?
	echo
	return $RETVAL
}	

rhstatus() {
	status plusbd
}	

case "$1" in
  start)
  	start
	;;
  stop)
  	stop
	;;
  restart)
  	restart
	;;
  reload)
  	reload
	;;
  status)
  	rhstatus
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|reload|status}"
	exit 1
esac

exit $?
