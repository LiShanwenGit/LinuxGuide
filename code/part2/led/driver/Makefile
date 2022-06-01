KERN_DIR = /home/lsw/licheepi/nanopi/linux-5.7.1
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko

obj-m += led_dev.o 



