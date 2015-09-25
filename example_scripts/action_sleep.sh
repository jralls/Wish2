#!/bin/sh
#
ver="`dirname $0`/"
. ${ver}definitions.sh

echo off > $frontflood
echo off > $backflood
echo off > $extbreakfast
echo off > $gym
echo off > $foyer
echo off > $backporch

#echo on > $frontgarage
echo on > $backgarage
echo on > $extbasement
