#
# Makefile for the linux x10-subsystem.
#
# (C)  wsh@sprintmail.com
# 
# This file is GPL.
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
O_TARGET := x10.ko
obj-m := x10.o

x10-objs := dev.o strings.o

KDIR	:= /lib/modules/$(shell uname -r)/build
MDIR	:= /lib/modules/$(shell uname -r)/kernel/drivers/char/x10
PWD	:= $(shell pwd)

CFLAGS_dev.o := -DCREATE_PROC

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	mkdir -p $(MDIR)
	cp -f $(O_TARGET) $(MDIR)

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

