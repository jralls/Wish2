#
#
# $Id: Makefile,v 1.1 2004/01/01 21:35:23 whiles Exp whiles $
#
INC=-I/usr/src/linux/include -I..
CC=gcc
LD=ld

all: .depend nbread x10watch x10logd nbecho

x10logd: x10logd.c Makefile
	$(CC) $(INC) $@.c -o $@

nbread: nbread.c Makefile
	$(CC) $(INC) $@.c -o $@

nbecho: nbecho.c Makefile
	$(CC) $(INC) $@.c -o $@

x10watch: x10watch.c Makefile
	$(CC) $(INC) $@.c -o $@

clean:
	rm nbread x10watch x10logd
	rm -f .depend
	rm -f .depend.bak

depend:	.depend

dep:	.depend

.c:
	co $@

.c.o:
	$(CC) $(INC) $*.c -o $*

.depend: x10logd.c nbread.c x10watch.c Makefile
	gccmakedep -f .depend -- $(CFLAGS) -- *.c

ifeq (.depend,$(wildcard .depend))
include .depend
endif
