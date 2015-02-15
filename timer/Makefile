obj-m += timer.o

KDIR := /usr/src/linux-headers-3.13.0-43-generic/

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules