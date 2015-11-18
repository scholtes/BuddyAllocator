obj-m := driver-07.o

CDIR := /lib/modules/$(shell uname -r)/build
MDIR := $(shell pwd)

all:
	make -C $(CDIR) M=$(MDIR) modules

clean:
	make -C $(CDIR) M=$(MDIR) clean