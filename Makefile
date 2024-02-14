device_name = hd44780
obj-m += $(device_name).o
dts_name = $(device_name)-i2c
PWD := $(CURDIR) 
 
all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	dtc -@ -I dts -O dtb -o $(dts_name).dtbo $(dts_name).dts
 
dt: $(dts_name).dts
	dtc -@ -I dts -O dtb -o $(dts_name).dtbo $(dts_name).dts
 
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf $(device_name).dtbo
