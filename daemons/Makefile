#
#
# $Id: Makefile,v 1.4 2004/02/21 02:23:30 whiles Exp whiles $
#
INCLUDE=-I/usr/src/linux/include -I../dev
CC=gcc
LD=gcc
CFLAGS=-D_REENTRANT -static $(INCLUDE)
LDFLAGS=pthread

KVER	:= $(shell uname -r | cut -c 1-3)

daemons := pld cm11ad

ifeq ($(KVER),2.6)
daemons	+= plusbd
endif

all: $(daemons)

cm11ad: main.o cm11a_xcvr.o
	$(LD) -lpthread -o $@ $+

pld: main.o pl_xcvr.o
	$(LD) -lpthread -o $@ $+

plusbd: main.o plusb_xcvr.o
	$(LD) -lpthread -o $@ $+

cm17ad: main.o cm17a_xcvr.o
	$(LD) -lpthread -o $@ $+

clean:
	rm -f $(daemons)
	rm -f *.o
	rm -f .depend
	rm -f .depend.bak

.c:
	co $@

