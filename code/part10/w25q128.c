#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>          //含有ioremap函数iounmap函数
#include <asm/uaccess.h>     //含有copy_from_user函数和含有copy_to_user函数
#include <linux/device.h>    //含有类相关的设备函数
#include <linux/cdev.h>
#include <linux/platform_device.h> //包含platform函数
#include <linux/of.h>        //包含设备树相关函数
#include <linux/spi/spi.h>
#include <linux/kobject.h>   //包含sysfs文件系统对象类
#include <linux/sysfs.h>     //包含sysfs操作文件函数

#define ERASE_OPCODE 0x20  //擦除操作码
#define R_STS_OPCODE 0x35  //读状态操作码

static char tx_data[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static char rx_data[16] = {0};


struct spi_transfer xfer;
struct spi_device *w25qxx_spi;     //保存spi设备结构体

static ssize_t w25qxx_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf,"read status register=%d\n",rx_data[0]);
}

static ssize_t w25qxx_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret;
    int erase_addr=0x123456;
    tx_data[0]=ERASE_OPCODE;
    tx_data[1]=(erase_addr>>16)&0xff;
    tx_data[2]=(erase_addr>>8)&0xff;
    tx_data[3]=(erase_addr)&0xff;
    xfer.len = 4;
    ret = spi_sync_transfer(w25qxx_spi,&xfer,1);  //擦除操作
    if(ret)
    {
        printk(KERN_INFO"erase w25q128 failed!\n");
    }
    tx_data[0]=R_STS_OPCODE;
    ret = spi_write_then_read(w25qxx_spi,tx_data,1,rx_data,1);
    return count;
}

static DEVICE_ATTR(w25q128, 0660, w25qxx_show, w25qxx_store); //定义文件属性

static void w25qxx_init(void)
{
    xfer.tx_buf = tx_data;
    xfer.rx_buf = rx_data;
    xfer.len    = sizeof(tx_data);
    xfer.bits_per_word = 8;
    memset(rx_data,0,16);
}

static int w25qxx_probe(struct spi_device *spi)
{
    int ret;
    ret = device_create_file(&spi->dev,&dev_attr_w25q128);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO"create w25q128_dev file failed!\n");
        return -1;
    }
    w25qxx_spi = spi_dev_get(spi);
    w25qxx_init();
    return 0;
}

static int w25qxx_remove(struct spi_device *spi)
{
    device_remove_file(&spi->dev,&dev_attr_w25q128);//删除属性文件
    printk(KERN_INFO "exit sysfs w25qxx!\n");
    return 0;
}

static struct of_device_id w25qxx_match_table[] = {  
    {.compatible = "test,w25qxx",},  
}; 

static struct spi_device_id w25qxx_ids[] = {
    {.name = "w25q128",},
};

static struct spi_driver w25qxx_cdev = {  
    .driver = {  
        .name =  "w25qxx",  
        .owner = THIS_MODULE,  
        .of_match_table = w25qxx_match_table,
    },  
    .probe  = w25qxx_probe,  
    .remove = w25qxx_remove,  
    .id_table = w25qxx_ids,
};  

module_spi_driver(w25qxx_cdev);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com"); //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("w25qxx_driver");//简单的描述

