# hd44780 i2c for linux

hardware : hd44780 (pcf8574+1602a LCD 16x2 screen)

distribution : ubuntu for raspberry pi

version : 22.04

## Usage
First, you need to run the makefile.  It will generate the `.ko` `.dtbo` file.
```
make
```
Then, we need to add the device to device tree
```
cp hd44780-i2c.dtbo /boot/firmware/overlays/
```
And modify the /boot/firmware/config.txt  Add the following command to config.txt
```
dtoverlay=hd44780-i2c
```
Then, make sure that i2c is enable, you can use `raspi-config` to enable it.

Finally, you can insert the module
```
sudo insmod hd44780.ko
```
You can use the following command communicate to hd44780, it will print to the screen
```
echo string > /dev/hd44780_driver
```
