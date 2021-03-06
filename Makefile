#/*
#   This module is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as publised by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.
#
#   This module is distributed WITHOUT ANY WARRANTY; without even
#   the implied warranty of MERCHANTABILITY or FITNESS FOR A
#   PARTICULAR PURPOSE.  See the GNU Library General Public License
#   for more details.
#
#   You should have received a copy of the GNU Library General Public
#   License along with this library; if not, write to the Free
#   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
#   MA 02111-1307, USA
#
#   Should you need to contact the author, you can do so by email to
#   <wsh@sprintmail.com>.
#*/

all:
	(cd daemons; make)
	(cd utils; make)
	(cd dev; ./make.sh)

clean:
	(cd daemons; make clean)
	(cd utils; make clean)
	(cd dev; ./make.sh clean)

install:
	sh ./scripts/install.sh $(KERNELDIR)
	(cd dev; ./make.sh install)
	depmod

#ifneq (/dev/x10,$(wildcard /dev/x10))
#	sh ./scripts/makedev.sh
#endif

