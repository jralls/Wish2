#!/bin/sh
#
currentdir=`pwd`
cd /usr/src/linux-`uname -r`
text=`grep x10 drivers/char/Makefile`
if [ "$text" = "" ]
then
	echo "Patching drivers/char make files..."
	patch -b -u -p1 < $currentdir/$1
else
	echo "Kernel already patched..."
fi
echo "Copying source code for drivers..."
cp -f $currentdir/dev/*.[ch] drivers/char/x10/
cp -f $currentdir/dev/`uname -r | cut -c 1-3`/Makefile drivers/char/x10/
echo "**********************************************************"
echo "Kernel patched.  Make sure to turn on the X10 module in "
echo "the kernel make options (make menuconfig or make xconfig)"
echo "by selection Character Devices and then the X10 device at"
echo "the bottom of the list.  Then build the kernel and install"
echo "the modules."
echo "**********************************************************"
