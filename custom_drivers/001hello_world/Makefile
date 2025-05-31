obj-m := main.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

KERN_DIR = /home/tmn/myWorkspace/ldd/source/linux_4.19/
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules
clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help
copy: 
	scp main.ko debian@192.168.7.2:/home/debian/drivers/
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules