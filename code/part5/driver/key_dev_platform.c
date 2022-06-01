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
#include <linux/irq.h>       //含有IRQ_HANDLED和IRQ_TYPE_EDGE_RISING
#include <linux/interrupt.h> //含有request_irq、free_irq函数

static dev_t  btn_cdev_num;      //定义一个设备号
static struct cdev *btn_cdev;   //定义一个设备管理结构体指针
static struct class *btn_class; //定义一个设备类
static struct device *btn;      //定义一个设备

ssize_t  volatile *keyadc_ctrl;
ssize_t  volatile *keyadc_intc;
ssize_t  volatile *keyadc_ints;
ssize_t  volatile *keyadc_data;

static unsigned int ev_press;   //一个全局变量，记录中断事件状态
DECLARE_WAIT_QUEUE_HEAD(btn_waitq);//注册一个等待队列button_waitq，用宏来申明一个全局变量
static unsigned int key_value = 0; //定义一个变量保存按键值

static irqreturn_t btn_irq(int irq, void *dev_id)
{
    wake_up_interruptible(&btn_waitq);   /* 唤醒休眠的进程，即调用read函数的进程 */
    ev_press = 1;
    *keyadc_ints |= ((1<<0)|(1<<1) | (1<<2) |(1<<3)|(1<<4));
    return IRQ_HANDLED;
}

static int btn_cdev_open (struct inode * inode, struct file * file)
{
    *keyadc_intc |=  ((1<<0)|(1<<1) | (1<<2) |(1<<3)|(1<<4));
    *keyadc_ctrl |= (1<<0);
    return 0;
}

static int btn_cdev_close(struct inode * inode, struct file * file)
{
    return 0;
}

static ssize_t btn_cdev_read(struct file * file, char __user * userbuf, size_t count, loff_t * off)
{
    int ret;
    unsigned int adc_value;
    adc_value = *(keyadc_data);
    //将当前进程放入等待队列button_waitq中,并且释放CPU进入睡眠状态
    wait_event_interruptible(btn_waitq, ev_press!=0);
    ret = copy_to_user(userbuf, &adc_value, 4);//将取得的按键值传给上层应用
    
    printk(KERN_WARNING"key adc = %d\n",(adc_value&(0x3f)));
    printk(KERN_WARNING"key statue = 0x%x\n",*(keyadc_ints));
    ev_press = 0;//按键已经处理可以继续睡眠
    if(ret)
    {
        printk("copy error\n");
        return -1;
    }
    return 1;
}

static struct file_operations btn_cdev_ops = {
        .owner = THIS_MODULE,
        .open  = btn_cdev_open,
        .read  = btn_cdev_read,
        .release = btn_cdev_close,
};

static int btn_probe(struct platform_device *pdev)
{
    int ret;
    struct resource *res;
       
    btn_cdev = cdev_alloc();    //动态申请一个设备结构体
    if(btn_cdev == NULL)
    {
        printk(KERN_WARNING"cdev_alloc failed!\n");
        return -1;
    }
    ret = alloc_chrdev_region(&btn_cdev_num,0,1,"button");  //动态申请一个设备号
    if(ret !=0)
    {
        printk(KERN_WARNING"alloc_chrdev_region failed!\n");
        return -1;
    }
    btn_cdev->owner = THIS_MODULE;  //初始化设备管理结构体的owner为THIS_MODULE
    btn_cdev->ops = &btn_cdev_ops;        //初始化设备操作函数指针为led_ops函数
    cdev_add(btn_cdev,btn_cdev_num,1);  //将设备添加到内核中
    btn_class = class_create(THIS_MODULE, "button");  //创建一个名为led_class的类 
    if(btn_class == NULL)
    {
        printk(KERN_WARNING"btn_class failed!\n");
        return -1;
    }
    btn = device_create(btn_class,NULL,btn_cdev_num,NULL,"button0");//创建一个设备名led0
    if(IS_ERR(btn))
    {
        printk(KERN_WARNING"device_create failed!\n");
        return -1;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);  //获取device中的LRADC_CTRL
    keyadc_ctrl = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);  //获取device中的LRADC_INTC
    keyadc_intc = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);  //获取device中的LRADC_INTS
    keyadc_ints = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 3);  //获取device中的LRADC_DATA
    keyadc_data = ioremap(res->start,(res->end - res->start)+1);

    ret = devm_request_irq(&pdev->dev, platform_get_irq(pdev, 0), btn_irq, 0, "mykey", (void*)&key_value);
    if(ret)
    {
        return ret;
    }
    return 0;
}

static int btn_remove(struct platform_device *pdev)
{
    iounmap(keyadc_ctrl);   //取消LRADC_CTRL映射
    iounmap(keyadc_intc);   //取消LRADC_INTC映射
    iounmap(keyadc_ints);   //取消LRADC_INTS映射
    iounmap(keyadc_data);    //取消LRADC_DATA映射
    cdev_del(btn_cdev);  //从内核中删除设备管理结构体

    unregister_chrdev_region(btn_cdev_num,1); //注销设备号
    device_destroy(btn_class,btn_cdev_num);   //删除设备节点
    class_destroy(btn_class);                 //删除设备类
    return 0;
}

static struct of_device_id btn_match_table[] = {  
    {.compatible = "mykey",},  
}; 

static struct platform_device_id btn_device_ids[] = {
    {.name = "btn",},
};

static struct platform_driver btn_driver=  
{  
    .probe  = btn_probe,  
    .remove = btn_remove,  
    .driver={  
        .name = "btn",  
        .of_match_table = btn_match_table,  
    },  
    .id_table = btn_device_ids,
}; 

module_platform_driver(btn_driver);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("btn_driver");  //简单的描述

