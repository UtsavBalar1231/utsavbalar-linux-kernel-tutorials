# If KERNELRELEASE is defined, we've been called from the kernel build system
ifneq ($(KERNELRELEASE),)
    obj-m := hello.o

# Otherwise, we're being called directly from the command line
else
    # Path to the kernel headers
    KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

endif 