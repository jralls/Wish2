#
#
# $Id: Makefile,v 1.5 2003/02/24 17:25:03 whiles Exp $
#
INC=-I/usr/src/linux/include -I..
CC=gcc
LD=ld

all: x10d

x10d: x10d.c Makefile
	$(CC) $(INC) $@.c -o $@

clean:
	rm x10d

.c:
	co $@

.c.o:
	$(CC) $(INC) $*.c -o $*
