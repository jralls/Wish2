#
#
# $Id: Makefile,v 1.3 2004/02/19 04:48:35 whiles Exp whiles $
#
INCLUDE=-I/usr/src/linux/include -I../dev
CC=gcc
LD=gcc
CFLAGS=-D_REENTRANT -static $(INCLUDE)
LDFLAGS=pthread

all: .depend pld cm11ad plusbd

cm11ad: main.o cm11a_xcvr.o
	$(LD) -lpthread -o $@ $+

pld: main.o pl_xcvr.o
	$(LD) -lpthread -o $@ $+

plusbd: main.o plusb_xcvr.o
	$(LD) -lpthread -o $@ $+

cm17ad: main.o cm17a_xcvr.o
	$(LD) -lpthread -o $@ $+

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
         ../dev/x10.h Makefile
	gccmakedep -f .depend -- $(CFLAGS) -- *.c

ifeq (.depend,$(wildcard .depend))
include .depend
endif
