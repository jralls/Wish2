#!/bin/sh
#
#
ver=`uname -r`
moduledir="/lib/modules/$ver/kernel/drivers/char/x10"

if [ ! -d $moduledir ]
then
	echo "mkdir $moduledir"
	mkdir $moduledir
fi

# earlier versions of this script put x10attach in /usr/bin
if [ -f /usr/bin/x10attach ]
then
	rm -f /usr/bin/x10attach
fi

echo "cp -f utils/x10attach /usr/sbin/"
cp -f utils/x10attach /usr/sbin/
echo "cp -f utils/x10logd /usr/sbin/"
cp -f utils/x10logd /usr/sbin/
echo "cp -f utils/nbread /usr/bin/"
cp -f utils/nbread /usr/bin/
echo "cp -f utils/x10watch /usr/bin/"
cp -f utils/x10watch /usr/bin/
echo "cp -f x10_pl.o x10_cm11a.o x10_cm17a.o x10_plusb.o $moduledir"
cp -f x10_pl.o x10_cm11a.o x10_cm17a.o x10_plusb.o $moduledir
echo "depmod -a $ver"
depmod -a $ver

