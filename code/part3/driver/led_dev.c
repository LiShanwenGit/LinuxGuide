#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>          //含有ioremap函数iounmap函数
#include <asm/uaccess.h>     //含有copy_from_user函数和含有copy_to_user函数
#include <linux/device.h>    //含有类相关的设备函数
#include <linux/cdev.h>
#include <linux/of.h>        //设备树的of函数

static dev_t led_dev_num;      //定义一个设备号
static struct cdev *led_dev;   //定义一个设备管理结构体指针
static struct class *led_class; //定义一个设备类
static struct device *led0;    //定义一个设备

size_t *gpioe_cfg0;   //存储虚拟地址到物理地址映射
size_t *gpioe_cfg1;   //存储虚拟地址到物理地址映射
size_t *gpioe_data;   //存储虚拟地址到物理地址映射
size_t *gpioe_pul0;   //存储虚拟地址到物理地址映射

static int led_open(struct inode *inode, struct file *file)
{
    /* GPIOE配置 */
    *((volatile size_t*)gpioe_cfg1) &= ~(7<<8); //清除配置寄存器
    *((volatile size_t*)gpioe_cfg1) |= (1<<8);  //配置GPIOE10为输出模式
    *((volatile size_t*)gpioe_pul0) &= ~(3<<20); //清除上/下拉寄存器
    *((volatile size_t*)gpioe_pul0) |= (1<<20);  //配置GPIOE10为上拉模式
    printk(KERN_DEBUG"open led!!!\n");
    return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG"close led!!!\n");
    return 0;
}

static int led_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    int ret;
    size_t status = *((volatile size_t*)gpioe_data);//获取GPIOE10状态
    ret = copy_to_user(buff,&status,4);  //将内核空间拷贝到用户空间buff
    if(ret < 0)
        printk(KERN_DEBUG"read error!!!\n");   //输出信息
    else
        printk(KERN_DEBUG"read led ok!!!\n");   //输出信息
    return 0;
}

static int led_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    int ret;
    size_t status; 
    ret = copy_from_user(&status,buff,4);     //将用户空间拷贝到内核空间的status
    if(ret < 0)
        printk(KERN_DEBUG"write error!!!\n");   //输出信息
    else
        printk(KERN_DEBUG"write led ok!!!\n");   //输出信息
    *((volatile size_t*)gpioe_data) &= ~(1<<10) ;//清除GPIOE10状态
    if(status)
        *((volatile size_t*)gpioe_data) |= (1<<10);//设置GPIOE10状态1
    return 0;
}

static struct file_operations led_ops = {
        .owner = THIS_MODULE,
        .open  = led_open,
        .read  = led_read,
        .write = led_write,
        .release = led_close,
};

static int __init led_init(void)   
{
    int ret;
    u32 GPIOE_CFG0;
    u32 GPIOE_CFG1;
    u32 GPIOE_DATA;
    u32 GPIOE_PUL0;
    struct device_node *led_node; //定义一个led节点
    led_dev = cdev_alloc();    //动态申请一个设备结构体
    if(led_dev == NULL)
    {
        printk(KERN_WARNING"cdev_alloc failed!\n");
            return -1;
    }
    ret = alloc_chrdev_region(&led_dev_num,0,1,"led");  //动态申请一个设备号
    if(ret !=0)
    {
        printk(KERN_WARNING"alloc_chrdev_region failed!\n");
            return -1;
    }
    led_dev->owner = THIS_MODULE;  //初始化设备管理结构体的owner为THIS_MODULE
    led_dev->ops = &led_ops;        //初始化设备操作函数指针为led_ops函数
    cdev_add(led_dev,led_dev_num,1);  //将设备添加到内核中
    led_class = class_create(THIS_MODULE, "led_class");  //创建一个类 
    if(led_class == NULL)
    {
        printk(KERN_WARNING"led_class failed!\n");
            return -1;
    }
    led0 = device_create(led_class,NULL,led_dev_num,NULL,"led0");   //创建一个设备
    if(IS_ERR(led0))
    {
        printk(KERN_WARNING"device_create failed!\n");
            return -1;
    }

    led_node = of_find_node_by_path("/led@0x01C20800"); //查找节点名为led@0x01C20800的节点
    if(led_node == NULL)
    {
        goto no_led_node;
    }
    if(of_property_read_u32_index(led_node,"reg",0,&GPIOE_CFG0)) //查找节点的reg属性的第0个值
    {
        goto of_error;
    }
    if(of_property_read_u32_index(led_node,"reg",2,&GPIOE_CFG1)) //查找节点的reg属性的第2个值
    {
        goto of_error;
    }
    if(of_property_read_u32_index(led_node,"reg",4,&GPIOE_DATA)) //查找节点的reg属性的第4个值
    {
        goto of_error;
    }
    if(of_property_read_u32_index(led_node,"reg",6,&GPIOE_PUL0)) //查找节点的reg属性的第6个值
    {
        goto of_error;
    }
    gpioe_cfg0 = ioremap(GPIOE_CFG0,4);  //将GPIOE_CFG0物理地址映射为虚拟地址
    gpioe_cfg1 = ioremap(GPIOE_CFG1,4);  //将GPIOE_CFG1物理地址映射为虚拟地址
    gpioe_data = ioremap(GPIOE_DATA,4);  //将GPIOE_DATA物理地址映射为虚拟地址
    gpioe_pul0 = ioremap(GPIOE_PUL0,4);  //将GPIOE_PUL0物理地址映射为虚拟地址
    return 0;

of_error:
    printk(KERN_WARNING"get reg failed!\n");
    return -1;
no_led_node:
    printk(KERN_WARNING"NO led node!\n");
    return -1;
}

static void __exit led_exit(void)   
{
    cdev_del(led_dev);  //从内核中删除设备管理结构体
    unregister_chrdev_region(led_dev_num,1);  //注销设备号
    device_destroy(led_class,led_dev_num);    //删除设备节点
    class_destroy(led_class);                 //删除设备类
    iounmap(gpioe_cfg0);  //取消GPIOE_CFG0映射
    iounmap(gpioe_cfg1);  //取消GPIOE_CFG1映射
    iounmap(gpioe_data);  //取消GPIOE_DATA映射
    iounmap(gpioe_pul0);  //取消GPIOE_PUL0映射
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("led_dev");  //简单的描述

