#
#
# $Id: Makefile,v 1.1 2004/01/01 21:35:23 whiles Exp whiles $
#
INC=-I/usr/src/linux/include -I../include
CC=gcc
LD=gcc
CFLAGS=-D_REENTRANT -static
LDFLAGS=pthread

all: .depend pld

cm11ad: main.o cm11a_xcvr.o
	$(LD) -o $@ $+

pld: main.o pl_xcvr.o
	$(LD) -lpthread -o $@ $+

plusbd: main.o plusb_xcvr.o
	$(LD) -o $@ $+

cm17ad: main.o cm17a_xcvr.o
	$(LD) -o $@ $+

clean:
	rm -f cm11ad pld plusbd cm17ad
	rm -f *.o
	rm -f .depend
	rm -f .depend.bak

depend:	.depend

dep:	.depend

.c:
	co $@

#.c.o:
#	$(CC) $(INC) $*.c -o $*

.depend: main.c pl_xcvr.c \
         ../include/x10.h Makefile
	gccmakedep -f .depend -- $(CFLAGS) -- *.c

ifeq (.depend,$(wildcard .depend))
include .depend
endif
