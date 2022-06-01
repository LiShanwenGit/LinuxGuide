#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>          //含有ioremap函数iounmap函数
#include <asm/uaccess.h>     //含有copy_from_user函数和含有copy_to_user函数
#include <linux/device.h>    //含有类相关的设备函数
#include <linux/cdev.h>
#include <linux/kobject.h>   //包含sysfs文件系统对象类
#include <linux/sysfs.h>     //包含sysfs操作文件函数
#include <linux/slab.h>
#include <linux/string.h>

#define  GPIOE_CFG0   (0x01C20890)
#define  GPIOE_CFG1   (0x01C20894)
#define  GPIOE_DATA   (0x01C208A0)
#define  GPIOE_PUL0   (0x01C208AC)

size_t *gpioe_cfg0;   //存储虚拟地址到物理地址映射
size_t *gpioe_cfg1;   //存储虚拟地址到物理地址映射
size_t *gpioe_data;   //存储虚拟地址到物理地址映射
size_t *gpioe_pul0;   //存储虚拟地址到物理地址映射

static int led_status = 0;  //定义一个led状态变量

static struct kobject *led_kobj;  //定义一个led_kobj

static ssize_t led_show(struct kobject* kobjs,struct kobj_attribute *attr,char *buf)
{
    printk(KERN_INFO "Read led\n");
    return sprintf(buf,"The led_status status is = %d\n",led_status);
}

static ssize_t led_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
    printk(KERN_INFO "led status store\n");
    if(0 == memcmp(buf,"on",2))
    {
        *((volatile size_t*)gpioe_data) &= ~(1<<12) ;//清除GPIOE12状态
        led_status = 1;
    }
    else if(0 == memcmp(buf,"off",3))
    {
        *((volatile size_t*)gpioe_data) |= (1<<12);//设置GPIOE12状态1
        led_status = 0;
    }
    else
    {
        printk(KERN_INFO "Not support cmd\n");
    }
    return count;
}
 
//定义一个led_status对象属性
static struct kobj_attribute led_attr = __ATTR(led_status,0660,led_show,led_store); 

static int __init sysfs_led_init(void)
{
    int ret;
    led_kobj = kobject_create_and_add("sys_led",NULL);
    if(led_kobj == NULL)
    {
        printk(KERN_INFO"create led_kobj failed!\n");
        return -1;
    }
    ret = sysfs_create_file(led_kobj, &led_attr.attr);
    if(ret != 0)
    {
        printk(KERN_INFO"create sysfa file failed!\n");
        return -1;
    }
    gpioe_cfg0 = ioremap(GPIOE_CFG0,4);  //将GPIOE_CFG0物理地址映射为虚拟地址
    gpioe_cfg1 = ioremap(GPIOE_CFG1,4);  //将GPIOE_CFG1物理地址映射为虚拟地址
    gpioe_data = ioremap(GPIOE_DATA,4);  //将GPIOE_DATA物理地址映射为虚拟地址
    gpioe_pul0 = ioremap(GPIOE_PUL0,4);  //将GPIOE_PUL0物理地址映射为虚拟地址

    *((volatile size_t*)gpioe_cfg1) &= ~(7<<16); //清除配置寄存器
    *((volatile size_t*)gpioe_cfg1) |= (1<<16);  //配置GPIOE12为输出模式
    *((volatile size_t*)gpioe_pul0) &= ~(3<<16); //清除上/下拉寄存器
    *((volatile size_t*)gpioe_pul0) |= (1<<12);  //配置GPIOE12为上拉模式

    *((volatile size_t*)gpioe_data) &= ~(1<<12) ;//清除GPIOE12状态
    return 0;
}

static void __exit sysfs_led_exit(void)
{
    sysfs_remove_file(led_kobj,&led_attr.attr); //删除属性
    kobject_put(led_kobj);  //删除对象
    printk(KERN_INFO "exit sysfs led!\n");
}
 
module_init(sysfs_led_init);
module_exit(sysfs_led_exit);

MODULE_LICENSE("GPL");              
MODULE_AUTHOR("1477153217@qq.com");      
MODULE_DESCRIPTION("sysfs led");  
MODULE_VERSION("0.1"); 

