#
# Makefile for the linux x10-subsystem.
#
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

O_TARGET	:= x10.o
obj-y		:= dev.o strings.o
obj-m	  	:= $(O_TARGET)
x10-objs	:= $(obj-y)

KDIR	:= /lib/modules/$(shell uname -r)/build
MDIR	:= /lib/modules/$(shell uname -r)/kernel/drivers/char/x10
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	mkdir -p $(MDIR)
	cp -f $(O_TARGET) $(MDIR)

clean:
	rm -f .*.o.flags
	rm -f *.o

include $(KDIR)/Rules.make
