#!/bin/sh
#
# $Id: makedev.sh,v 1.3 2003/01/06 15:53:36 whiles Exp root $
#
MAJOR_DATA=120
MAJOR_CONTROL=121
if [ $# -eq 2 ]
then
	MAJOR_DATA=$1
	MAJOR_CONTROL=$2
elif [ $# -ne 0 ]
then
	echo "Syntax $0 [data_major control_major]"
	exit 1
fi

echo "MAJOR_DATA=$MAJOR_DATA"
echo "MAJOR_CONTROL=$MAJOR_CONTROL"
exit 1

if [ ! -d /dev/x10 ]; then mkdir /dev/x10; fi

for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
# uncomment the following if you prefer to have "0" in front of the lower
# numbered units.
#  if [ $j -lt 10 ]; then f="0"; fi
  if [ $j -ge 10 ]; then f=""; fi
  mknod /dev/x10/a$f$j c $MAJOR_DATA `expr $j - 1`
  mknod /dev/x10/b$f$j c $MAJOR_DATA `expr $j + 15`
  mknod /dev/x10/c$f$j c $MAJOR_DATA `expr $j + 31`
  mknod /dev/x10/d$f$j c $MAJOR_DATA `expr $j + 47`
  mknod /dev/x10/e$f$j c $MAJOR_DATA `expr $j + 63`
  mknod /dev/x10/f$f$j c $MAJOR_DATA `expr $j + 79`
  mknod /dev/x10/g$f$j c $MAJOR_DATA `expr $j + 95`
  mknod /dev/x10/h$f$j c $MAJOR_DATA `expr $j + 111`
  mknod /dev/x10/i$f$j c $MAJOR_DATA `expr $j + 127`
  mknod /dev/x10/j$f$j c $MAJOR_DATA `expr $j + 143`
  mknod /dev/x10/k$f$j c $MAJOR_DATA `expr $j + 159`
  mknod /dev/x10/l$f$j c $MAJOR_DATA `expr $j + 175`
  mknod /dev/x10/m$f$j c $MAJOR_DATA `expr $j + 191`
  mknod /dev/x10/n$f$j c $MAJOR_DATA `expr $j + 207`
  mknod /dev/x10/o$f$j c $MAJOR_DATA `expr $j + 223`
  mknod /dev/x10/p$f$j c $MAJOR_DATA `expr $j + 239`
done
# read the status of all 16 units on a housecode with a headerline
  mknod /dev/x10/a c $MAJOR_CONTROL 0
  mknod /dev/x10/b c $MAJOR_CONTROL 1
  mknod /dev/x10/c c $MAJOR_CONTROL 2
  mknod /dev/x10/d c $MAJOR_CONTROL 3
  mknod /dev/x10/e c $MAJOR_CONTROL 4
  mknod /dev/x10/f c $MAJOR_CONTROL 5
  mknod /dev/x10/g c $MAJOR_CONTROL 6
  mknod /dev/x10/h c $MAJOR_CONTROL 7
  mknod /dev/x10/i c $MAJOR_CONTROL 8
  mknod /dev/x10/j c $MAJOR_CONTROL 9
  mknod /dev/x10/k c $MAJOR_CONTROL 10
  mknod /dev/x10/l c $MAJOR_CONTROL 11
  mknod /dev/x10/m c $MAJOR_CONTROL 12
  mknod /dev/x10/n c $MAJOR_CONTROL 13
  mknod /dev/x10/o c $MAJOR_CONTROL 14
  mknod /dev/x10/p c $MAJOR_CONTROL 15
# read the status of all 16 units on a housecode without a headerline
  mknod /dev/x10/araw c $MAJOR_CONTROL 16
  mknod /dev/x10/braw c $MAJOR_CONTROL 17
  mknod /dev/x10/craw c $MAJOR_CONTROL 18
  mknod /dev/x10/draw c $MAJOR_CONTROL 19
  mknod /dev/x10/eraw c $MAJOR_CONTROL 20
  mknod /dev/x10/fraw c $MAJOR_CONTROL 21
  mknod /dev/x10/graw c $MAJOR_CONTROL 22
  mknod /dev/x10/hraw c $MAJOR_CONTROL 23
  mknod /dev/x10/iraw c $MAJOR_CONTROL 24
  mknod /dev/x10/jraw c $MAJOR_CONTROL 25
  mknod /dev/x10/kraw c $MAJOR_CONTROL 26
  mknod /dev/x10/lraw c $MAJOR_CONTROL 27
  mknod /dev/x10/mraw c $MAJOR_CONTROL 28
  mknod /dev/x10/nraw c $MAJOR_CONTROL 29
  mknod /dev/x10/oraw c $MAJOR_CONTROL 30
  mknod /dev/x10/praw c $MAJOR_CONTROL 31

# all: read the status of all housecodes and units
mknod /dev/x10/statusraw c $MAJOR_CONTROL 32
mknod /dev/x10/status c $MAJOR_CONTROL 33
# changed: get array of any changes to status
mknod /dev/x10/changedraw c $MAJOR_CONTROL 34
mknod /dev/x10/changed c $MAJOR_CONTROL 35

mknod /dev/x10/log c $MAJOR_CONTROL 48
mknod /dev/x10/edata c $MAJOR_CONTROL 64
