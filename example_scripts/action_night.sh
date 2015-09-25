#!/bin/sh
#

ver="`dirname $0`/"
. ${ver}definitions.sh

case "$1" in
  on)
	action="on"
	;;
  off)
	action="off"
	;;
  *)
	action="on"
	;;
esac

echo $action > $frontflood
echo $action > $backflood
#echo $action > $extbreakfast
#echo $action > $frontgarage
echo $action > $extbasement
echo $action > $foyer
if [ "$action" = "on" ]
then
	echo ps18 > $foyer
	echo ps18 > $foyer
fi

