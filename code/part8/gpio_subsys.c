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
#include <linux/kobject.h>   //包含sysfs文件系统对象类
#include <linux/sysfs.h>     //包含sysfs操作文件函数
#include <linux/gpio/consumer.h> //包含gpio子系统接口
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>    //包含gpio一些宏


struct gpio_desc *led_pin;    //gpio资源


static ssize_t led_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf,"led status=%d\n",gpiod_get_value(led_pin));
}

static ssize_t led_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    if(0 == memcmp(buf,"on",2))
    {
        gpiod_set_value(led_pin,0);
    }
    else if(0 == memcmp(buf,"off",3))
    {
        gpiod_set_value(led_pin,1);
    }
    else
    {
        printk(KERN_INFO "Not support cmd\n");
    }
    return count;
}

static DEVICE_ATTR(led, 0660, led_show, led_store); //定义文件属性


static int led_probe(struct platform_device *pdev)
{
    int ret;
 	ret = device_create_file(&pdev->dev,&dev_attr_led);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO"create w25q128_dev file failed!\n");
        return -1;
    }
    led_pin = devm_gpiod_get(&pdev->dev,"led",GPIOF_OUT_INIT_LOW);
    if(IS_ERR(led_pin))
    {
        printk(KERN_ERR"Get gpio resource failed!\n");
        return -1;
    }
    gpiod_direction_output(led_pin,0);

    return 0;
}

static int led_remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev,&dev_attr_led);//删除属性文件
    devm_gpiod_put(&pdev->dev,led_pin);
    printk(KERN_INFO "exit sysfs sys_led!\n");
    return 0;
}

static struct of_device_id led_match_table[] = {  
    {.compatible = "test,led_subsys",},  
}; 

static struct platform_device_id led_ids[] = {
    {.name = "led_subsys",},
};

static struct platform_driver led_cdev = {  
    .driver = {  
        .name =  "led_subsys",  
        .owner = THIS_MODULE,  
        .of_match_table = led_match_table,
    },  
    .probe  = led_probe,  
    .remove = led_remove,  
    .id_table = led_ids,
};  

module_platform_driver(led_cdev);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("led_cdev");  //简单的描述

