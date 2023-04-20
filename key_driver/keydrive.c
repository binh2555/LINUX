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
#include <linux/mutex.h>

#define OE_OFFSET              0x134
#define SETDATAOUT_OFFSET      0x194
#define CLEARDATAOUT_OFFSET    0x190
#define DATA_IN_OFFSET         0x138
DEFINE_MUTEX(phim_lock);


void __iomem *base_addr = NULL;
uint32_t my_reg[2];   //mang de chua thong tin cua truong reg duoc mo ta trong device tree
uint32_t my_row[4];   //mang de chua thong tin cua truong key_row
uint32_t my_collum[4];  //mang de chua thong tin cua truong key_collum
int now_status[4][4];
int last_status[4][4];
int phim = -1;
int flag = 0;

struct timer_list my_timer;

static const struct of_device_id matrix_key_of_match[] = {
    {.compatible = "matrixkey_b"},
    {},
};

static char convert(int a)
{
    switch(a) {
        case 0 : 
            return '1';
        case 1 :
            return '2';
        case 2 :
            return '3';
        case 3 : 
            return 'A';
        case 4 :
            return '4';
        case 5 :
            return '5';
        case 6 : 
            return '6';
        case 7 :
            return 'B';
        case 8 :
            return '7';
        case 9 : 
            return '8';
        case 10 :
            return '9';
        case 11 :
            return 'C';
    }
    return 'n';

}

static void timer_function(struct timer_list *t)
{
    //mutex_lock(&phim_lock);
    uint32_t mi = 0;    //bien tam de luu trang thai nut nhan

    // if(flag == 0)
    // {
    //     phim = -1;
    // }
    // else flag = 0;

    int i=0,j=0;
    //viet chuong trinh quet ban phim

    //B1:doc gia tri hien tai cua tat ca cac phim tren ban phim
    for(i=0;i<3;i++)
    {
        writel_relaxed((1<<my_row[i]), base_addr + SETDATAOUT_OFFSET);  //dua row1 len muc cao
        mi = readl_relaxed(base_addr + DATA_IN_OFFSET);         //doc gia tri thanh ghi DATA_IN va luu vao bien tam mi
        //pr_info("mi = %08x\n",mi);
        for( j=0;j<4;j++)
        {
            //cap nhat gia tri cho mang chua trang thai hien tai cua nut nhan
            if((mi & (1<<my_collum[j])) != 0)
            {
                now_status[i][j] = 1;
                //pr_info("phim thu hang %d cot %d da duoc nhan\n",i,j);
            }
            else
            {
                now_status[i][j] = 0;
            }
        }
        writel_relaxed((1<<my_row[i]), base_addr + CLEARDATAOUT_OFFSET);  //dua row1 ve muc thap
        
    }

    //B2:quet tat ca cac mang chua trang thai nut nhan de biet nut nao vua duoc nhan roi dua nó vào bien cho cac ung dung tang user su dung
    for( i=0;i<3;i++)
    {
        for( j=0;j<4;j++)
        {
            if(now_status[i][j] == 1 && last_status[i][j] == 0)
            {
                phim = (i*4) + j;
                flag = 1;
                break;
            }
        }
    }

    //B3:cap nhat lai mang chua trang thai cuoi cung cua nut nhan
    for( i=0;i<3;i++)
    {
        for( j = 0;j<4;j++)
        {
            last_status[i][j] = now_status[i][j];
        }
    }
    // if(phim != -1)
    //     pr_info("phim = %d\n",phim);

    //mutex_unlock(&phim_lock);
    mod_timer(&my_timer, jiffies + 15);
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

    pr_info("%s, %d\n", __func__, __LINE__);
   // mutex_lock(&phim_lock);
    char po = convert(phim);
    char *data = &po;
    pr_info("data = %c\n",po);
    copy_to_user(buf,data,1);
    phim = -1;
   // mutex_unlock(&phim_lock);

    return 1;
}

static ssize_t misc_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *f_pos)
{
    pr_info("%s, %d\n", __func__, __LINE__);
    return 0;
}


struct file_operations misc_fops = {
    .owner = THIS_MODULE,
    .open = misc_open,
    .release = misc_release,
    .read = misc_read,
    .write = misc_write,
};

static struct miscdevice matrixkey = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "matrixkey",
    .fops = &misc_fops,
};

int matrix_key_driver_probe(struct platform_device *pdev)
{
    const struct device *dev = &pdev->dev;
    const struct device_node *np = dev->of_node;

    of_property_read_u32_array(np,"reg",my_reg,2);    //doc gia tri cua truong reg va luu vao mang my_reg
    pr_info("reg  = %08x %08x\n",my_reg[0],my_reg[1]);  

    base_addr = ioremap(0x44E07000,0x1000);          //map base_addr vao dia chi base cua gpio1
    if(base_addr == NULL)
    {
        pr_info("mapping that bai\n");
        return 0;
    }

    //config cac chan hang cua matrix key lam output va cac chan cot lam input
    of_property_read_u32_array(np,"key_r",my_row,4);        //doc gia tri truong key_r trong device tree
    pr_info("reg  = %d %d %d %d\n",my_row[0],my_row[1],my_row[2],my_row[3]);
    
    uint32_t re = 0;
    re = readl_relaxed(base_addr+OE_OFFSET);           //doc gia tri cua thanh ghi oe va luu vao bien tam re 
    re &= ((~(1<<my_row[0])) & (~(1<<my_row[1])) & (~(1<<my_row[2])));        //ghi gia tri de config cac chan hang lam output vao bien tam

    of_property_read_u32_array(np,"key_c",my_collum,4);     //doc gia tri truong key_collum trong device tree
    re |= ((1<<my_collum[0]) | (1<<my_collum[1]) | (1<<my_collum[2]) | (1<<my_collum[3]));        //ghi gia tri de config cac chan cot lam input vao bien tam
    pr_info("re = %08x\n",re);
    writel_relaxed(re, base_addr + OE_OFFSET);   //ghi gia tri bien tam re vao thanh ghi OE cua gpio1

    int i;
    //dua cac chan row ve gia tri mac dinh la 0
    for(i=0;i<3;i++)
    {
        writel_relaxed((1<<my_row[i]), base_addr + CLEARDATAOUT_OFFSET);
    }
   
    //cai dat thong so cho struct timer_list
    my_timer.expires = jiffies + HZ;
    my_timer.function = timer_function;
    timer_setup(&my_timer, timer_function, 0);
    add_timer(&my_timer);

    if (misc_register(&matrixkey) == 0)
    {
        pr_info("registered device file success\n");
    }
    else 
    {
        pr_info("registered device file failed\n");
    }

	return 0;
}

int matrix_key_driver_remove(struct platform_device *pdev)
{
    del_timer_sync(&my_timer);
    misc_deregister(&matrixkey);
    pr_info("driver remove\n");
	return 0;
}

static struct platform_driver matrix_key_driver = {
    .probe = matrix_key_driver_probe,
    .remove = matrix_key_driver_remove,
    .driver = {
        .name = "matrixkey_driver",
        .of_match_table = matrix_key_of_match,
    },
};

module_platform_driver(matrix_key_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("driver for matrix key 4*4");
