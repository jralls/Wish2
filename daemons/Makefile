#
#
# $Id: Makefile,v 1.5 2005/01/24 01:04:00 whiles Exp root $
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
	$(LD) -lpthread -Wall -o $@ $+

pld: main.o pl_xcvr.o
	$(LD) -lpthread -Wall -o $@ $+

plusbd: main.o plusb_xcvr.o
	$(LD) -lpthread -Wall -o $@ $+

cm17ad: main.o cm17a_xcvr.o
	$(LD) -lpthread -Wall -o $@ $+

clean:
	rm -f $(daemons)
	rm -f *.o
	rm -f .depend
	rm -f .depend.bak

.c:
	co $@

