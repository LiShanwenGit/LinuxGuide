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
#include <linux/regmap.h>


static struct regmap *oled_regmap;

 static uint8_t diaplay_buffer[128][8];

static int oled_write_cmd(uint8_t cmd)
{
    int ret;
    ret = regmap_write(oled_regmap,0x00,cmd); //DC=0 command
    if(ret)
    {
        return ret;
    }
    else
    {
        return 0;
    }
}

static int oled_write_data(uint8_t data)
{
    int ret;
    ret = regmap_write(oled_regmap,0x40,data); //DC=1 command
    if(ret)
    {
        return ret;
    }
    else
    {
        return 0;
    }
}

static void oled_on(void)
{
    oled_write_cmd(0x8D);  
    oled_write_cmd(0x14);  
    oled_write_cmd(0xAF);  
}

static void oled_off(void)
{
    oled_write_cmd(0x8D);  
    oled_write_cmd(0x10); 
    oled_write_cmd(0xAE);  
}

void oled_refresh(void)
{
    uint8_t i, j;
    for (i = 0; i < 8; i ++) 
    {  
        oled_write_cmd(0xB0 + i);    
        oled_write_cmd(0x02); 
        oled_write_cmd(0x10);     
        for (j = 0; j < 128; j ++) 
        {
            oled_write_data(diaplay_buffer[j][i]); 
        }
    }   
}

void oled_fill_screen(uint8_t data)  
{ 
    memset(diaplay_buffer,data, sizeof(diaplay_buffer));
    oled_refresh();
}

static void oled_init(void)
{
    oled_write_cmd(0xAE);//--turn off oled panel
    oled_write_cmd(0x00);//---set low column address
    oled_write_cmd(0x10);//---set high column address
    oled_write_cmd(0x40);//--set start line address 
    oled_write_cmd(0x81);//--set contrast control register
    oled_write_cmd(0xCF);// Set SEG Output Current Brightness
    oled_write_cmd(0xA1);//--Set SEG/Column Mapping
    oled_write_cmd(0xC0);//Set COM/Row Scan Direction
    oled_write_cmd(0xA6);//--set normal display
    oled_write_cmd(0xA8);//--set multiplex ratio(1 to 64)
    oled_write_cmd(0x3f);//--1/64 duty
    oled_write_cmd(0xD3);//-set display offset    Shift Mapping RAM Counter (0x00~0x3F)
    oled_write_cmd(0x00);//-not offset
    oled_write_cmd(0xd5);//--set display clock divide ratio/oscillator frequency
    oled_write_cmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
    oled_write_cmd(0xD9);//--set pre-charge period
    oled_write_cmd(0xF1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    oled_write_cmd(0xDA);//--set com pins hardware configuration
    oled_write_cmd(0x12);
    oled_write_cmd(0xDB);//--set vcomh
    oled_write_cmd(0x40);//Set VCOM Deselect Level
    oled_write_cmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
    oled_write_cmd(0x02);//
    oled_write_cmd(0x8D);//--set Charge Pump enable/disable
    oled_write_cmd(0x14);//--set(0x10) disable
    oled_write_cmd(0xA4);// Disable Entire Display On (0xa4/0xa5)
    oled_write_cmd(0xA6);// Disable Inverse Display On (0xa6/a7)
    oled_write_cmd(0xAF);//--turn on oled panel

    oled_on();
    oled_fill_screen(0xff);
}

static const struct regmap_config oled_config =
{
    .reg_bits = 8,  //寄存器8位
    .val_bits = 8,  //数据8位
    .max_register = 255,  //最大寄存器255个
    .cache_type = REGCACHE_NONE, //不使用cache
    .volatile_reg = false,
};

static ssize_t oled_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    oled_off();
    return 1;
}

static ssize_t oled_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    oled_fill_screen(*buf);
    return count;
}

static DEVICE_ATTR(oled, 0660, oled_show, oled_store); //定义文件属性

static int oled_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    ret = device_create_file(&client->dev,&dev_attr_oled);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO"create oled_dev file failed!\n");
        return -1;
    }
    oled_regmap = regmap_init_i2c(client,&oled_config);
    oled_init();
    return 0;
}

static int oled_remove(struct i2c_client *client)
{
    device_remove_file(&client->dev,&dev_attr_oled);//删除属性文件
    regmap_exit(oled_regmap);
    printk(KERN_INFO "exit sysfs oled!\n");
    return 0;
}

static const struct i2c_device_id oled_id[] = {
    { "test,oled", 0 },
    { }
};

static const struct of_device_id oled_of_match[] = {
    { .compatible = "test,oled"},
    { },
};

MODULE_DEVICE_TABLE(of, oled_of_match);

static struct i2c_driver oled_driver = {
    .driver = {
        .name = "test,oled",
        .owner = THIS_MODULE,  
        .of_match_table = oled_of_match,
    },
    .probe      = oled_probe,
    .remove = oled_remove,
    .id_table   = oled_id,
};

module_i2c_driver(oled_driver);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("oled_driver");  //简单的描述

