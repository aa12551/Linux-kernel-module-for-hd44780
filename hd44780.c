// mytest.c
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#define SLAVE_DEVICE_NAME   ("hd44780")

typedef enum { IR, DR } dest_reg;
#define BL	0x08
#define E	0x04
#define RW	0x02
#define RS	0x01

#define HD44780_CLEAR_DISPLAY	0x01
#define HD44780_RETURN_HOME	0x02
#define HD44780_ENTRY_MODE_SET	0x04
#define HD44780_DISPLAY_CTRL	0x08
#define HD44780_SHIFT		0x10
#define HD44780_FUNCTION_SET	0x20
#define HD44780_CGRAM_ADDR	0x40
#define HD44780_DDRAM_ADDR	0x80

#define HD44780_DL_8BITS	0x10
#define HD44780_DL_4BITS	0x00
#define HD44780_N_2LINES	0x08
#define HD44780_N_1LINE		0x00
#define HD44780_F_5x7       0x00
#define HD44780_F_5x10      0x04

#define HD44780_D_DISPLAY_ON	0x04
#define HD44780_D_DISPLAY_OFF	0x00
#define HD44780_C_CURSOR_ON	0x02
#define HD44780_C_CURSOR_OFF	0x00
#define HD44780_B_BLINK_ON	0x01
#define HD44780_B_BLINK_OFF	0x00

#define HD44780_ID_INCREMENT	0x02
#define HD44780_ID_DECREMENT	0x00
#define HD44780_S_SHIFT_ON	0x01
#define HD44780_S_SHIFT_OFF	0x00

static struct i2c_client *hd44780_client = NULL;

static int hd44780_probe(struct i2c_client *client, \
                const struct i2c_device_id *hd44780_id);

static int hd44780_remove(struct i2c_client *client);

static const struct of_device_id hd44780_of_match_table[] = {
        {.compatible = SLAVE_DEVICE_NAME},
        {}
};
MODULE_DEVICE_TABLE(of, hd44780_of_match_table);

static struct i2c_driver hd44780_driver = {
        .driver = {
                .name   = SLAVE_DEVICE_NAME,
                .owner  = THIS_MODULE,
                .of_match_table = hd44780_of_match_table,
        },
        .probe          = hd44780_probe,
        .remove         = hd44780_remove,
};


static int hd44780_raw_write(const char data)
{
        return i2c_smbus_write_byte(hd44780_client, data);
}

static void hd44780_write_nibble(dest_reg reg, u8 data)
{
        /* Shift the interesting data on the upper 4 bits (b7-b4) */
        data = (data << 4) & 0xF0;

        /* Set the back light */
        data |= BL;

        /* Flip the RS bit if we write do data register */
        if (reg == DR)
                data |= RS;

        /* Keep the RW bit low, because we write */
        data = data | (RW & 0x00);

        /* Raise the E signal... */
        hd44780_raw_write(data | E);
        udelay(50);

        hd44780_raw_write(data);
        udelay(100);
}
static void hd44780_write_instruction_high_nibble(u8 data)
{
        u8 l = data & 0x0F;
        hd44780_write_nibble(IR, l);
}

static void hd44780_write_instruction(u8 data)
{
        u8 h = (data >> 4) & 0x0F;
        u8 l = data & 0x0F;

        hd44780_write_nibble(IR, h);
        hd44780_write_nibble(IR, l);
}

static void hd44780_write_data(u8 data)
{
        u8 h = (data >> 4) & 0x0F;
        u8 l = data & 0x0F;

        hd44780_write_nibble(DR, h);
        hd44780_write_nibble(DR, l);
}

static void hd44780_write_str(const char *str, int size)
{
        int i = 0;
        for(; i < size; i++)
                hd44780_write_data(str[i]);
}

static ssize_t hd44780_file_write(struct file *file, const char *buf, 
                size_t len, loff_t *offs)
{
        int copied, no_copied;
        char* data = kmalloc(len, GFP_KERNEL);
        no_copied = copy_from_user(data, buf, len);

        copied = len - no_copied;

        /* Display clear */
        hd44780_write_instruction(0x01);
        mdelay(2);

        /* write str */
        hd44780_write_str(data, copied - 1);

        return len - no_copied;
}

static const struct file_operations hd44780_fops = {
        .owner = THIS_MODULE,
        .write = hd44780_file_write
};

struct miscdevice hd44780_misc_device = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "hd44780_misc",
        .fops = &hd44780_fops,
};

static int hd44780_probe(struct i2c_client *client, const struct i2c_device_id *hd44780_id)
{
        hd44780_client = client;

        if(misc_register(&hd44780_misc_device)) {
                printk("misc register failed\n");
                return -1;
        }

        mdelay(20);
        hd44780_write_instruction(0x30);

        mdelay(10);
        hd44780_write_instruction(0x30);

        udelay(20);
        hd44780_write_instruction(0x30);

        udelay(50);

        /* Function set (set 4 bit mode) */
        hd44780_write_instruction_high_nibble(HD44780_FUNCTION_SET \
        | HD44780_DL_4BITS);
        udelay(50);

        /* Function set */  
        hd44780_write_instruction(HD44780_FUNCTION_SET | HD44780_DL_4BITS \
        | HD44780_N_1LINE | HD44780_F_5x7);
        udelay(50);

        /* Display off */
        hd44780_write_instruction(HD44780_DISPLAY_CTRL | HD44780_D_DISPLAY_OFF);
        udelay(100);

        /* Display clear */
        hd44780_write_instruction(0x01);
        mdelay(2);

        /* Entry mode set */
        hd44780_write_instruction(HD44780_ENTRY_MODE_SET | HD44780_ID_INCREMENT \
        | HD44780_S_SHIFT_OFF);
        udelay(50);

        /* Turn on Display */
        hd44780_write_instruction(HD44780_DISPLAY_CTRL | HD44780_D_DISPLAY_ON \
        | HD44780_C_CURSOR_OFF | HD44780_B_BLINK_OFF);
        udelay(50);

        /* write something */
        hd44780_write_str("Hello", 5);
        printk("initial complete\n");
        return 0;
}

static int hd44780_remove(struct i2c_client *client) 
{
        misc_deregister(&hd44780_misc_device);

        printk("%s remove\n", SLAVE_DEVICE_NAME);
        return 0;
}

module_i2c_driver(hd44780_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("david");
