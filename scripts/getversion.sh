#!/bin/sh
#

if [ $# -eq 0 ]
then
	echo "Syntax $0 KERNELDIR"
	exit 1
fi
KERNELDIR="$1"
VERSION="`cat $KERNELDIR/Makefile | grep ^VERSION | awk '{print $3}'`"
PATCHLEVEL="`cat $KERNELDIR/Makefile | grep ^PATCHLEVEL | awk '{print $3}'`"
SUBLEVEL="`cat $KERNELDIR/Makefile | grep ^SUBLEVEL | awk '{print $3}'`"
EXTRAVERSION="`cat $KERNELDIR/Makefile | grep ^EXTRAVERSION | awk '{print $3}'`"
echo "${VERSION}.${PATCHLEVEL}.${SUBLEVEL}${EXTRAVERSION}"
