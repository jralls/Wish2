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
obj-m := x10.o

x10-objs := dev.o strings.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
