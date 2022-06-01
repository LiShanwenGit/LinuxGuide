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
#include <linux/input.h>
#include <linux/irq.h>

struct input_dev *mykeypad_dev;           //定义一个input_dev结构体  

ssize_t  volatile *keyadc_ctrl;
ssize_t  volatile *keyadc_intc;
ssize_t  volatile *keyadc_ints;
ssize_t  volatile *keyadc_data;

static unsigned int key_value = 0; //定义一个变量保存按键值

static irqreturn_t  mykeypad_irq (int irq, void *dev_id)       //中断服务函数
{
    /*上报事件*/
    if(((*keyadc_ints)&(1<<1)) != 0)  //按键按下
    { 
        key_value = ((*keyadc_data)&(0x3f));   //获取按键值

        if(key_value == 24)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_A , 1); //上报EV_KEY类型,1(按下)    
        }
        else if(key_value == 17)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_B, 1);  //上报EV_KEY类型, 1(按下)    
        }
        else if(key_value == 11)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_C, 1);  //上报EV_KEY类型, 1(按下)    
        }
        else
        {
            input_event(mykeypad_dev, EV_KEY, KEY_D, 1);  //上报EV_KEY类型, 1(按下)    
        }
    }
    else if(((*keyadc_ints)&(1<<4)) != 0) //按键释放
    {
        if(key_value == 24)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_A , 0);  //上报EV_KEY类型, 0(释放)    
        }
        else if(key_value == 17)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_B, 0);  //上报EV_KEY类型, 0(释放)   
        }
        else if(key_value == 11)
        {
            input_event(mykeypad_dev, EV_KEY, KEY_C, 0);  //上报EV_KEY类型, 0(释放)    
        }
        else
        {
            input_event(mykeypad_dev, EV_KEY, KEY_D, 0);  //上报EV_KEY类型, 0(释放)    
        }
    }

    input_sync(mykeypad_dev);       // 上传同步事件,告诉系统有事件出现
    *keyadc_ints |= ((1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4));
    return IRQ_HANDLED;
}

static int mykeypad_probe(struct platform_device *pdev)
{
    int ret;      
    struct resource *res;
    mykeypad_dev=input_allocate_device();      //向内核申请input_dev结构体
    set_bit(EV_KEY,mykeypad_dev->evbit);       //支持键盘事件
    set_bit(EV_REP,mykeypad_dev->evbit);       //支持键盘重复按事件
    set_bit(KEY_A,mykeypad_dev->keybit);       //支持按键 A
    set_bit(KEY_B,mykeypad_dev->keybit);       //支持按键 B
    set_bit(KEY_C,mykeypad_dev->keybit);       //支持按键 C
    set_bit(KEY_D,mykeypad_dev->keybit);       //支持按键 D

    mykeypad_dev->name = pdev->name;           //设备名称
    mykeypad_dev->phys = "mykeypad/input0";    //设备文件路径
    mykeypad_dev->open = NULL;                 //设备打开操作函数
    mykeypad_dev->close = NULL;                //设备关闭操作函数
    mykeypad_dev->id.bustype = BUS_HOST;       //设备总线类型
    mykeypad_dev->id.vendor = 0x0001;          //设备厂家编号
    mykeypad_dev->id.product = 0x0001;         //设备产品编号
    mykeypad_dev->id.version = 0x0100;         //设备版本

    ret = input_register_device(mykeypad_dev);       //注册input_dev
    if(ret)
    {
        input_free_device(mykeypad_dev);
        printk(KERN_ERR "regoster input device failed!\n");
        return ret;
    }
    mykeypad_dev->name="mykeypad";
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);//获取device中的LRADC_CTRL
    keyadc_ctrl = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);//获取device中的LRADC_INTC
    keyadc_intc = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);//获取device中的LRADC_INTS
    keyadc_ints = ioremap(res->start,(res->end - res->start)+1);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 3);//获取device中的LRADC_DATA
    keyadc_data = ioremap(res->start,(res->end - res->start)+1);

    ret = devm_request_irq(&pdev->dev, platform_get_irq(pdev, 0), mykeypad_irq, 0, "mykey", (void*)&key_value);
    *keyadc_ctrl&=0;
    *keyadc_ctrl |= (1<<12);  //单次模式，其他默认
    *keyadc_intc |= (1<<4) | (1<<1);
    *keyadc_ctrl |= (1<<0);
    if(ret)
    {
        return ret;
    }
    return 0;
}

static int mykeypad_remove(struct platform_device *pdev)
{
    iounmap(keyadc_ctrl);   //取消LRADC_CTRL映射
    iounmap(keyadc_intc);   //取消LRADC_INTC映射
    iounmap(keyadc_ints);   //取消LRADC_INTS映射
    iounmap(keyadc_data);   //取消LRADC_DATA映射

    input_unregister_device(mykeypad_dev);     //卸载类下的驱动设备
    input_free_device(mykeypad_dev);           //释放驱动结构体
    return 0;
}

static struct of_device_id mykeypad_match_table[] = {  
    {.compatible = "mykeypad",},  
}; 

static struct platform_device_id mykeypad_device_ids[] = {
    {.name = "mykeypad",},
};

static struct platform_driver mykeypad_driver=  
{  
    .probe  = mykeypad_probe,  
    .remove = mykeypad_remove,  
    .driver={  
        .name = "mykeypad",  
        .of_match_table = mykeypad_match_table,  
    },  
    .id_table = mykeypad_device_ids,
};

module_platform_driver(mykeypad_driver);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("mykeypad_driver");  //简单的描述

