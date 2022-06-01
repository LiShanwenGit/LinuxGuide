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
#include <linux/i2c.h>
#include <linux/kobject.h>   //包含sysfs文件系统对象类
#include <linux/sysfs.h>     //包含sysfs操作文件函数

static char recv[16] = {0};  //保存接收数据
struct i2c_client *at24c02_dev;    //定义一个i2c设备结构体指针

static ssize_t at24c02_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    int ret;
    ret = i2c_smbus_write_byte(at24c02_dev,0x02); //先发写地址操作
    if(!ret)
    {
        printk(KERN_ERR"write addr failed!\n");
    }
    recv[0] = i2c_smbus_read_byte(at24c02_dev); //然后发读数据操作
    return sprintf(buf,"read data = 0x%x\n",recv[0]);
}

static ssize_t at24c02_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    int ret;
    char data = 0x2f;
    ret = i2c_smbus_write_i2c_block_data(at24c02_dev,0x02,1,&data);//写一个字节数据
    if(ret < 0)
    {
        printk(KERN_ERR"write data failed!\n");
    }
    return count;
}

static DEVICE_ATTR(at24c02, 0660, at24c02_show, at24c02_store); //定义文件属性


static int at24c02_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    ret = device_create_file(&client->dev,&dev_attr_at24c02);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO"create at24c02_dev file failed!\n");
        return -1;
    }
    at24c02_dev = client; //初始化i2c设备结构体指针
    return 0;
}

static int at24c02_remove(struct i2c_client *client)
{
    device_remove_file(&client->dev,&dev_attr_at24c02);//删除属性文件
    printk(KERN_INFO "exit sysfs at24c02!\n");
    return 0;
}

static const struct i2c_device_id at24c02_id[] = {
    { "test,at24c0x", 0 },
    { }
};

static const struct of_device_id at24c02_of_match[] = {
    { .compatible = "test,at24c0x"},
    { },
};

MODULE_DEVICE_TABLE(of, at24c02_of_match);

static struct i2c_driver at24c02_driver = {
    .driver = {
        .name = "test,at24c0x",
        .owner = THIS_MODULE,  
        .of_match_table = at24c02_of_match,
    },
    .probe      = at24c02_probe,
    .remove = at24c02_remove,
    .id_table   = at24c02_id,
};

module_i2c_driver(at24c02_driver);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("at24c02_driver");  //简单的描述

