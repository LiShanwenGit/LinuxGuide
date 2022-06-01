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
#include <linux/miscdevice.h> //包含misc相关函数

size_t *gpioe_cfg0;   //存储虚拟地址到物理地址映射
size_t *gpioe_cfg1;   //存储虚拟地址到物理地址映射
size_t *gpioe_data;   //存储虚拟地址到物理地址映射
size_t *gpioe_pul0;   //存储虚拟地址到物理地址映射

static int misc_led_open(struct inode *inode, struct file *file)
{
    /* GPIOE配置 */
    *((volatile size_t*)gpioe_cfg1) &= ~(7<<16); //清除配置寄存器
    *((volatile size_t*)gpioe_cfg1) |= (1<<16);  //配置GPIOE12为输出模式
    *((volatile size_t*)gpioe_pul0) &= ~(3<<16); //清除上/下拉寄存器
    *((volatile size_t*)gpioe_pul0) |= (1<<12);  //配置GPIOE12为上拉模式
    printk(KERN_DEBUG"open led!!!\n");
    return 0;
}

static int misc_led_close(struct inode *inode, struct file *filp)
{
    /* GPIOE配置 */
    printk(KERN_DEBUG"close led!!!\n");
    return 0;
}

static int misc_led_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    int ret;
    size_t status = *((volatile size_t*)gpioe_data);//获取GPIOE12状态
    ret = copy_to_user(buff,&status,4);  //将内核空间拷贝到用户空间buff
    if(ret < 0)
        printk(KERN_DEBUG"read error!!!\n");   //输出信息
    else
        printk(KERN_DEBUG"read led ok!!!\n");   //输出信息
    return 0;
}

static int misc_led_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    int ret;
    size_t status; 
    ret = copy_from_user(&status,buff,4);     //将用户空间拷贝到内核空间的status
    if(ret < 0)
        printk(KERN_DEBUG"write error!!!\n");   //输出信息
    else
        printk(KERN_DEBUG"write led ok!!!\n");   //输出信息
    *((volatile size_t*)gpioe_data) &= ~(1<<12) ;//清除GPIOE12状态
    if(status)
        *((volatile size_t*)gpioe_data) |= (1<<12);//设置GPIOE12状态1
    return 0;
}

static struct file_operations misc_led_ops = {
        .owner = THIS_MODULE,
        .open  = misc_led_open,
        .read  = misc_led_read,
        .write = misc_led_write,
        .release = misc_led_close,
};

static struct miscdevice misc_led_dev = {
    .minor  = MISC_DYNAMIC_MINOR, //动态分配次设备号
    .name   = "miscled",
    .fops   = &misc_led_ops, //文件操作集
};

static int misc_led_probe(struct platform_device *pdev)
{
    struct resource *res;  
    misc_register(&misc_led_dev);  //注册misc设备
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);  //获取device中的GPIOE_CFG0
    gpioe_cfg0 = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);  //获取device中的GPIOE_CFG1
    gpioe_cfg1 = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);  //获取device中的GPIOE_DATA
    gpioe_data = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 3);  //获取device中的GPIOE_PUL0
    gpioe_pul0 = ioremap(res->start,(res->end - res->start)+1);

    return 0;
}

static int misc_led_remove(struct platform_device *pdev)
{
    iounmap(gpioe_cfg0);  //取消GPIOE_CFG0映射
    iounmap(gpioe_cfg1);  //取消GPIOE_CFG1映射
    iounmap(gpioe_data);  //取消GPIOE_DATA映射
    iounmap(gpioe_pul0);  //取消GPIOE_PUL0映射
    misc_deregister(&misc_led_dev);  //注销misc设备

    return 0;
}

static struct of_device_id misc_led_match_table[] = {  
    {.compatible = "lite200,misc_led",},  
}; 

static struct platform_device_id misc_led_device_ids[] = {
    {.name = "misc_led",},
};

static struct platform_driver misc_led_driver=  
{  
    .probe  = misc_led_probe,  
    .remove = misc_led_remove,  
    .driver={  
        .name = "misc_led",  
        .of_match_table = misc_led_match_table,  
    },  
    .id_table = misc_led_device_ids,
}; 

static int misc_led_driver_init(void)
{
    platform_driver_register(&misc_led_driver);
    return 0;
}

static void misc_led_driver_exit(void)
{
    platform_driver_unregister(&misc_led_driver);
}

module_init(misc_led_driver_init);
module_exit(misc_led_driver_exit);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("misc_led_driver");  //简单的描述

