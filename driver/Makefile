SDK_DIR = /root/duo-buildroot-sdk
KERN_DIR = $(SDK_DIR)/linux_5.10/build/cv1800b_milkv_duo_sd

all:
	make -C $(KERN_DIR) M=$(PWD) modules

clean:
	make -C $(KERN_DIR) M=$(PWD) modules clean
	rm -rf modules.order

obj-m += ssd1306.o