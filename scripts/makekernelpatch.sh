#!/bin/sh
#
origkernel="linux-`uname -r`"
newkernel="linux-`uname -r`-x10"


cd /usr/src/
diff -Nru $origkernel/drivers/char $newkernel/drivers/char > /tmp/x10-`uname -r`-patch

