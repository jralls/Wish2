#!/bin/sh
#
# chkconfig: - 99 99
# description: Starts and stops the X10 control interface \
#
ver=`uname -r`
moduledir="/lib/modules/$ver/kernel/drivers/char/x10"

if [ ! -f x10.o ]
then 
	echo "You must make the objects first.  Run make"
	exit 1
fi

if [ ! -d $moduledir ]
then
	echo "mkdir $moduledir"
	mkdir $moduledir
fi

echo "cp -f utils/x10attach /usr/bin/"
cp -f utils/x10attach /usr/bin/
echo "cp -f utils/nbread /usr/bin/"
cp -f utils/nbread /usr/bin/
echo "cp -f *.o $moduledir"
cp -f *.o $moduledir
echo "depmod -a $ver"
depmod -a $ver

