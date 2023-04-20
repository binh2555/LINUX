#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/time.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#define OE_OFFSET              0x134
#define SETDATAOUT_OFFSET      0x194
#define CLEARDATAOUT_OFFSET    0x190
#define DATA_IN_OFFSET         0x138

uint32_t my_pin[4];
uint32_t my_reg[2];
uint32_t status[4] = {0};   //mang chua trang thai cua led
void __iomem *base_addr = NULL;


//Dang ky thong tin device tree node ma driver muon dieu khien
static const struct of_device_id blink_led_of_match[] = {
    {.compatible = "led_device_0"},
    {},
};

void set_led(int pin_x)
{
    if(status[pin_x] == 0)
    {
        status[pin_x] = 1;
        writel_relaxed((1<<my_pin[pin_x]), base_addr + SETDATAOUT_OFFSET);
    }
    else
    {
        status[pin_x] = 0;
        writel_relaxed((1<<my_pin[pin_x]), base_addr + CLEARDATAOUT_OFFSET);
    }
}

int misc_open(struct inode *node, struct file *filep)
{
    pr_info("%s, %d\n", __func__, __LINE__);

    return 0;
}

int misc_release(struct inode *node, struct file *filep)
{
    pr_info("%s, %d\n", __func__, __LINE__);

    return 0;
}

static ssize_t misc_read(struct file *filp, char __user *buf, size_t count,
                         loff_t *f_pos)
{


    return 1;
}

static ssize_t misc_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *f_pos)
{
    pr_info("%s, %d\n", __func__, __LINE__);
    char data[10];
    memset(data,0,sizeof(data));
    copy_from_user(data,buf,count);

    if(data[0] != 'n' && data[0] != 0)
    {
        switch (data[0]){
            case '1' :
                set_led(0);
                break;
            case '2' :
                set_led(1);
                break;
            case '3' :
                set_led(2);
                break;
            case '4' :
                set_led(3);
                break;
        }
        data[0] = 0;
    }

    return 0;
}

struct file_operations misc_fops = {
    .owner = THIS_MODULE,
    .open = misc_open,
    .release = misc_release,
    .read = misc_read,
    .write = misc_write,
};

static struct miscdevice led_device_f = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "led_device_0",
    .fops = &misc_fops,
};


// Trong ham probe nhap nhay led luon
int blink_led_driver_probe(struct platform_device *pdev)
{
    pr_info("probe is runing...\n");
    const struct device *dev = &pdev->dev;
    const struct device_node *np = dev->of_node;

    of_property_read_u32_array(np,"reg",my_reg,2);    //doc gia tri cua truong reg va luu vao mang my_reg
    pr_info("reg  = %08x %08x\n",my_reg[0],my_reg[1]); 

    of_property_read_u32_array(np,"pin_num",my_pin,4);        //doc gia tri truong pin_num trong device tree
    pr_info("pin_num  = %d %d %d %d\n",my_pin[0],my_pin[1],my_pin[2],my_pin[3]);


    base_addr = ioremap(my_reg[0],my_reg[1]);          //map base_addr vao dia chi base cua gpio2
    if(base_addr == NULL)
    {
        pr_info("mapping that bai\n");
        return 0;
    }

    uint32_t re = 0;
    re = readl_relaxed(base_addr+OE_OFFSET);           //doc gia tri cua thanh ghi oe va luu vao bien tam re
    re &= ((~(1<<my_pin[0])) & (~(1<<my_pin[1])) & (~(1<<my_pin[2])) & (~(1<<my_pin[3])));        //ghi gia tri de config cac chan led lam output vao bien tam
    writel_relaxed(re, base_addr + OE_OFFSET);   //ghi gia tri bien tam re vao thanh ghi OE cua gpio2

    int i;
    //dua cac chan led ve gia tri mac dinh la 0 dong thoi dua gia tri cua mang status ve gia tri mac dinh la 0
    for(i=0;i<3;i++)
    {
        writel_relaxed((1<<my_pin[i]), base_addr + CLEARDATAOUT_OFFSET);
        status[i] = 0;
    }

    for(i=0;i<3;i++)
    {
        writel_relaxed((1<<my_pin[i]), base_addr + SETDATAOUT_OFFSET);
        status[i] = 0;
    }

    msleep(3000);
     //dua cac chan led ve gia tri mac dinh la 0 dong thoi dua gia tri cua mang status ve gia tri mac dinh la 0
    for(i=0;i<3;i++)
    {
        writel_relaxed((1<<my_pin[i]), base_addr + CLEARDATAOUT_OFFSET);
        status[i] = 0;
    }
    

    pr_info("probe is called\n");

    if (misc_register(&led_device_f) == 0)
    {
        pr_info("registered device file success\n");
    }
    else 
    {
        pr_info("registered device file failed\n");
    }

   
    return 0;
}

int blink_led_driver_remove(struct platform_device *pdev)
{
  
    misc_deregister(&led_device_f);
    pr_info("driver remove\n");

    return 0;
}

static struct platform_driver blink_led_driver = {
    .probe = blink_led_driver_probe,
    .remove = blink_led_driver_remove,
    .driver = {
        .name = "blink_led",
        .of_match_table = blink_led_of_match,
    },
};

module_platform_driver(blink_led_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Blink Led kernel module");
