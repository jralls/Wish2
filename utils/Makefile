#
#
# $Id: Makefile,v 1.3 2004/06/27 18:22:56 whiles Exp whiles $
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
INC=-I/usr/src/linux/include -I..
CC=gcc
LD=ld

all: nbread x10watch x10logd nbecho

x10logd: x10logd.c Makefile
	$(CC) $(INC) $@.c -o $@

nbread: nbread.c Makefile
	$(CC) $(INC) $@.c -o $@

nbecho: nbecho.c Makefile
	$(CC) $(INC) $@.c -o $@

x10watch: x10watch.c Makefile
	$(CC) $(INC) $@.c -o $@

clean:
	rm nbread x10watch x10logd nbecho
	rm -f .depend
	rm -f .depend.bak

.c:
	co $@

.c.o:
	$(CC) $(INC) $*.c -o $*
