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

echo "cp -f utils/x10attach /usr/bin/"
cp -f utils/x10attach /usr/bin/
echo "cp -f utils/nbread /usr/bin/"
cp -f utils/nbread /usr/bin/
echo "cp -f x10_pl.o x10_cm11a.o x10_cm17a.o x10_plusb.o $moduledir"
cp -f x10_pl.o x10_cm11a.o x10_cm17a.o x10_plusb.o $moduledir
echo "depmod -a $ver"
depmod -a $ver

