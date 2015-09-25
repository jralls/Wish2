#!/bin/sh
#

ver="`dirname $0`/"
. ${ver}definitions.sh

case "$1" in
  open)
	action="on"
	;;
  close)
	action="off"
	;;
  *)
	echo "Syntax $0 [open|close]"
	exit 1
	;;
esac

night=`/usr/bin/nbread $photocell`
if [ "$night" != "000" -o "$action" == "off" ]; then
	echo $action > $frontgarage
	echo $action > $livingroom
fi
echo $action > $dingdong
