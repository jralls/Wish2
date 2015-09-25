#!/bin/sh
#
cp -f `uname -r | cut -d '.' -f 1,2`/Makefile .
make $*
rm -f Makefile
