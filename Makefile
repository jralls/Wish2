all:
	(cd dev; make)
	(cd daemons; make)
	(cd utils; make)

clean:
	(cd dev; make clean)
	(cd daemons; make clean)
	(cd utils; make clean)

install:
	sh ./scripts/install.sh $(KERNELDIR)
ifneq (/dev/x10,$(wildcard /dev/x10))
	sh ./scripts/makedev.sh
endif

