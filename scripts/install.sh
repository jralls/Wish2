#!/bin/sh
#
#
#ver=`uname -r`
#if [ $# -ne 0 ]
#then
#	pwd=`dirname $0`
#	ver=`$pwd/getversion.sh "$1"`
#fi
#moduledir="/usr/local/lib/modules/$ver/kernel/drivers/char/x10"
#
#echo "Installing module in $ver"
#
#if [ ! -d $moduledir ]
#then
#	echo "mkdir $moduledir"
#	mkdir -p $moduledir
#fi

echo "cp -f daemons/*d /usr/sbin/"
cp -f daemons/*d /usr/sbin/
echo "cp -f utils/x10logd /usr/sbin/"
cp -f utils/x10logd /usr/sbin/
echo "cp -f utils/x10watch /usr/bin/"
cp -f utils/x10watch /usr/bin/
echo "cp -f utils/nbread /usr/bin/"
cp -f utils/nbread /usr/bin/
echo "cp -f utils/nbecho /usr/bin/"
cp -f utils/nbecho /usr/bin/
#echo "cp -f dev/x10.o $moduledir"
#cp -f dev/x10.o $moduledir/x10.ko
#cp -f dev/x10.o $moduledir/x10.o
if [ ! -f /etc/logrotate.d/x10 ]; then
  cp scripts/logrotate.d/x10 /etc/logrotate.d/x10
fi

if [ -d /etc/rc.d/init.d ]; then
	if [ -f /etc/rc.d/init.d/x10.cm11a ]; then
	echo "/etc/rc.d/init.d/x10.cm11a exists...not replacing"
	else
	cp -f example_scripts/x10.cm11a.sh /etc/rc.d/init.d/x10.cm11a
	fi

	if [ -f /etc/rc.d/init.d/x10.pl ]; then
	echo "/etc/rc.d/init.d/x10.pl exists...not replacing"
	else
	cp -f example_scripts/x10.pl.sh /etc/rc.d/init.d/x10.pl
	fi

	if [ -f /etc/rc.d/init.d/x10.plusb ]; then
	echo "/etc/rc.d/init.d/x10.plusb exists...not replacing"
	else
	cp -f example_scripts/x10.plusb.sh /etc/rc.d/init.d/x10.plusb
	fi
fi
