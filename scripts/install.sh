#!/bin/sh
#
#
ver=`uname -r`
if [ $# -ne 0 ]
then
pwd=`dirname $0`
ver=`$pwd/getversion.sh "$1"`
fi
moduledir="/lib/modules/$ver/kernel/drivers/char/x10"

echo "Installing objects in $ver"

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

echo "cp -f example_scripts/x10.cm11a.sh /usr/local/etc/x10_cm11a"
echo "cp -f example_scripts/x10.cm17a.sh /usr/local/etc/x10_cm17a"
echo "cp -f example_scripts/x10.plusb.sh /usr/local/etc/x10_plusb"
echo "cp -f example_scripts/x10.pl.sh /usr/local/etc/x10_pl"
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
echo "depmod -ae $ver"
depmod -ae $ver

