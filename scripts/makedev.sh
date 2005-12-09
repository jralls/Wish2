#!/bin/sh
#
# $Id: makedev.sh,v 1.8 2005/09/18 01:43:20 root Exp root $
#
MAJOR_DATA=120
MAJOR_CONTROL=121
DIR="/dev/x10"
if [ $# -eq 3 ]
then
	MAJOR_DATA=$1
	MAJOR_CONTROL=$2
	DIR="$3"
elif [ $# -ne 0 ]
then
	echo "Syntax $0 [data_major control_major basedir]"
	exit 1
fi

if [ -e /dev/.devfsd -a -x /sbin/devfsd ]
then
	echo "DEVFS currently running.  No need to create /dev devices"
	exit 0
fi	

if [ -d /dev/x10 ]
then
	echo "/dev/x10 already exists, not remaking devices"
	exit 0
fi

echo "Creating X10 devices in $DIR with data_major=$MAJOR_DATA and control_major=$MAJOR_CONTROL"

if [ ! -d $DIR ]; then mkdir $DIR; fi

for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
# uncomment the following if you prefer to have "0" in front of the lower
# numbered units.
#  if [ $j -lt 10 ]; then f="0"; fi
  if [ $j -ge 10 ]; then f=""; fi
  mknod $DIR/a$f$j c $MAJOR_DATA `expr $j - 1`
  mknod $DIR/b$f$j c $MAJOR_DATA `expr $j + 15`
  mknod $DIR/c$f$j c $MAJOR_DATA `expr $j + 31`
  mknod $DIR/d$f$j c $MAJOR_DATA `expr $j + 47`
  mknod $DIR/e$f$j c $MAJOR_DATA `expr $j + 63`
  mknod $DIR/f$f$j c $MAJOR_DATA `expr $j + 79`
  mknod $DIR/g$f$j c $MAJOR_DATA `expr $j + 95`
  mknod $DIR/h$f$j c $MAJOR_DATA `expr $j + 111`
  mknod $DIR/i$f$j c $MAJOR_DATA `expr $j + 127`
  mknod $DIR/j$f$j c $MAJOR_DATA `expr $j + 143`
  mknod $DIR/k$f$j c $MAJOR_DATA `expr $j + 159`
  mknod $DIR/l$f$j c $MAJOR_DATA `expr $j + 175`
  mknod $DIR/m$f$j c $MAJOR_DATA `expr $j + 191`
  mknod $DIR/n$f$j c $MAJOR_DATA `expr $j + 207`
  mknod $DIR/o$f$j c $MAJOR_DATA `expr $j + 223`
  mknod $DIR/p$f$j c $MAJOR_DATA `expr $j + 239`
done
# read the status of all 16 units on a housecode with a headerline
  mknod $DIR/a c $MAJOR_CONTROL 0
  mknod $DIR/b c $MAJOR_CONTROL 1
  mknod $DIR/c c $MAJOR_CONTROL 2
  mknod $DIR/d c $MAJOR_CONTROL 3
  mknod $DIR/e c $MAJOR_CONTROL 4
  mknod $DIR/f c $MAJOR_CONTROL 5
  mknod $DIR/g c $MAJOR_CONTROL 6
  mknod $DIR/h c $MAJOR_CONTROL 7
  mknod $DIR/i c $MAJOR_CONTROL 8
  mknod $DIR/j c $MAJOR_CONTROL 9
  mknod $DIR/k c $MAJOR_CONTROL 10
  mknod $DIR/l c $MAJOR_CONTROL 11
  mknod $DIR/m c $MAJOR_CONTROL 12
  mknod $DIR/n c $MAJOR_CONTROL 13
  mknod $DIR/o c $MAJOR_CONTROL 14
  mknod $DIR/p c $MAJOR_CONTROL 15
# read the status of all 16 units on a housecode without a headerline
  mknod $DIR/araw c $MAJOR_CONTROL 16
  mknod $DIR/braw c $MAJOR_CONTROL 17
  mknod $DIR/craw c $MAJOR_CONTROL 18
  mknod $DIR/draw c $MAJOR_CONTROL 19
  mknod $DIR/eraw c $MAJOR_CONTROL 20
  mknod $DIR/fraw c $MAJOR_CONTROL 21
  mknod $DIR/graw c $MAJOR_CONTROL 22
  mknod $DIR/hraw c $MAJOR_CONTROL 23
  mknod $DIR/iraw c $MAJOR_CONTROL 24
  mknod $DIR/jraw c $MAJOR_CONTROL 25
  mknod $DIR/kraw c $MAJOR_CONTROL 26
  mknod $DIR/lraw c $MAJOR_CONTROL 27
  mknod $DIR/mraw c $MAJOR_CONTROL 28
  mknod $DIR/nraw c $MAJOR_CONTROL 29
  mknod $DIR/oraw c $MAJOR_CONTROL 30
  mknod $DIR/praw c $MAJOR_CONTROL 31

# all: read the status of all housecodes and units
mknod $DIR/statusraw c $MAJOR_CONTROL 32
mknod $DIR/status c $MAJOR_CONTROL 33
# changed: get array of any changes to status
mknod $DIR/changedraw c $MAJOR_CONTROL 34
mknod $DIR/changed c $MAJOR_CONTROL 35

mknod $DIR/log c $MAJOR_CONTROL 48
mknod $DIR/edata c $MAJOR_CONTROL 64

mknod $DIR/.api c $MAJOR_CONTROL 240

if [ -d /etc/udev/devices ]; then
  cp -arp $DIR /etc/udev/devices/
  echo "*********************************************************"
  echo "* You appear to be using udev.  A copy of the devices   *"
  echo "* have been placed in /etc/udev/devices so that they    *"
  echo "* automatically be created whenever the machine starts. *"
  echo "*********************************************************"
fi
