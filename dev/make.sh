#!/bin/sh
#
cp -f `uname -r | cut -c 1-3`/Makefile .
make $*
