MODULES = proc_sample
KERNEL_SOURCE = /lib/modules/$(shell uname -r)/build
ARCH = x86_64
CURDIR = $(shell pwd)
CROSS_COMPILE = 
CFLAGS =

default: all
obj-m += $(MODULES:%=%.o)
BUILD = $(MODULES:%=%.ko)

all:: $(BUILD)
	
clean:
	rm -f $(BUILD) *.o *.ko *.mod.c *.mod.o *~ .*.cmdModule.symvers
	rm -rf .tmp_versions
$(MODULES:%=%.ko):*c
	$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) M=$(CURDIR) modules

