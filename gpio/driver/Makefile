export ARCH:=arm
export CROSS_COMPILE=$(TOOLCHAIN)

obj-m += am335x_gpio.o
 
KDIR := $(KERN_DIR)
PWD := $(shell pwd)
CC:=$(TOOLCHAIN_CC)


default:
	$(MAKE) -C $(KDIR)  M=$(PWD) modules 
 
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
