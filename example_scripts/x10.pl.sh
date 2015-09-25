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


start() {
	echo -n $"Starting X10 services: "
	if [ -f /var/log/x10log ]
	then
		rm -f /var/log/x10log
	fi
	modprobe x10
	/usr/sbin/pld -device /dev/ttyS0  
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
	killproc pld -HUP
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
	killproc pld -HUP
	RETVAL=$?
	echo
	return $RETVAL
}	

rhstatus() {
	status pld
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
