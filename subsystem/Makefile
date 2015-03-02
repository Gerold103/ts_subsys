obj-m += ts_subsys.o

KDIR := /usr/src/linux-headers-3.13.0-45-generic/

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules