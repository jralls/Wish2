#!/bin/sh
#
DIR=2.4
VERSION=`uname -r | cut -d '.' -f 1,2`
MAJOR=`echo $VERSION | cut -d '.' -f 1`
MINOR=`echo $VERSION | cut -d '.' -f 2`
if [ $MAJOR -ge 3 -a $MINOR -ge 10 ]; then
  DIR=3.16
elif [ $MAJOR -ge 3 -o $MAJOR -eq 2 -a $MINOR -ge 6 ]; then
  DIR=2.6
fi

cp $DIR/Makefile .
make $*
rm -f Makefile
