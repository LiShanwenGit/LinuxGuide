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
#include <linux/fb.h>        //包含frame buffer
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/gpio.h>

static struct fb_info *myfb;           //定义一个fb_info
static struct spi_device *fbspi;       //保存spi驱动指针
static struct gpio_desc *dc_pin;       //dc引脚
static struct gpio_desc *reset_pin;    //reset引脚

static u32 pseudo_palette[16];  //调色板缓存区

static void fb_write_reg(struct spi_device *spi, u8 reg)
{
    gpiod_set_value(dc_pin, 0); //低电平，命令
    spi_write(spi, &reg, 1);
}

static void fb_write_data(struct spi_device *spi, u16 data)
{   
    u8 buf[2];
    buf[0] = ((u8)(data>>8));
    buf[1] = ((u8)(data&0x00ff));
    gpiod_set_value(dc_pin, 1);  //高电平，数据
    spi_write(spi, &buf[0], 1);
    spi_write(spi, &buf[1], 1);   
}

static void fb_set_win(struct spi_device *spi, u8 xStar, u8 yStar,u8 xEnd,u8 yEnd)
{
    fb_write_reg(spi,0x2a);   
    fb_write_data(spi,xStar);
    fb_write_data(spi,xEnd);
    fb_write_reg(spi,0x2b);   
    fb_write_data(spi,yStar);
    fb_write_data(spi,yEnd);
    fb_write_reg(spi,0x2c); 
}


static void myfb_init(struct spi_device *spi)
{
    gpiod_set_value(reset_pin, 0); //设低电平
    msleep(100);
    gpiod_set_value(reset_pin, 1); //设高电平
    msleep(50);
        /* 写寄存器，初始化 */
    fb_write_reg(spi,0x36);
    fb_write_data(spi,0x0000);
    fb_write_reg(spi,0x3A);
    fb_write_data(spi,0x0500);
    fb_write_reg(spi,0xB2);
    fb_write_data(spi,0x0C0C);
    fb_write_data(spi,0x0033);
    fb_write_data(spi,0x3300);
    fb_write_data(spi,0x0033);
    fb_write_data(spi,0x3300);
    fb_write_reg(spi,0xB7);
    fb_write_data(spi,0x3500);
    fb_write_reg(spi,0xB8);
    fb_write_data(spi,0x1900);
    fb_write_reg(spi,0xC0);
    fb_write_data(spi,0x2C00);
    fb_write_reg(spi,0xC2);
    fb_write_data(spi,0xC100);
    fb_write_reg(spi,0xC3);
    fb_write_data(spi,0x1200);
    fb_write_reg(spi,0xC4);
    fb_write_data(spi,0x2000);
    fb_write_reg(spi,0xC6);
    fb_write_data(spi,0x0F00);
    fb_write_reg(spi,0xD0);
    fb_write_data(spi,0xA4A1);
    fb_write_reg(spi,0xE0);
    fb_write_data(spi,0xD004);
    fb_write_data(spi,0x0D11);
    fb_write_data(spi,0x132B);
    fb_write_data(spi,0x3F54);
    fb_write_data(spi,0x4C18);
    fb_write_data(spi,0x0D0B);
    fb_write_data(spi,0x1F23);
    fb_write_reg(spi,0xE1);
    fb_write_data(spi,0xD004);
    fb_write_data(spi,0x0C11);
    fb_write_data(spi,0x132C);
    fb_write_data(spi,0x3F44);
    fb_write_data(spi,0x512F);
    fb_write_data(spi,0x1F1F);
    fb_write_data(spi,0x2023);
    fb_write_reg(spi,0x21);
    fb_write_reg(spi,0x11);
    mdelay(50);  
    fb_write_reg(spi,0x29);
    mdelay(200);

}

void fb_refresh(struct fb_info *fbi, struct spi_device *spi)
{
    u8 *p = (u8 *)(fbi->screen_base);
    fb_set_win(spi, 0,0,239,239);
    gpiod_set_value(dc_pin, 1);  //高电平，数据
    spi_write(spi, p, 240*240*2);
}

static ssize_t myfb_write(struct fb_info *info, const char __user *buf,size_t count, loff_t *ppos)
{
	unsigned long total_size;
	unsigned long p = *ppos;
	void *dst;
	total_size = info->fix.smem_len;
	if (p > total_size)
		return -EINVAL;
	if (count + p > total_size)
		count = total_size - p;
	if (!count)
		return -EINVAL;
	dst = info->screen_buffer + p;

	if (copy_from_user(dst, buf, count))
		return -EFAULT;
	fb_refresh(info,fbspi);
	*ppos += count;
	return count;
}

static void myfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	sys_fillrect(info, rect);
	fb_refresh(info,fbspi);
}

static void myfb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	sys_copyarea(info, area);
	fb_refresh(info,fbspi);
}

static void myfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	sys_imageblit(info, image);
	fb_refresh(info,fbspi);
}

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

static u32 pseudo_palette[16];

static int myfb_setcolreg(u32 regno, u32 red,u32 green, u32 blue,u32 transp, struct fb_info *info)
{
    unsigned int val;
    
    if (regno > 16)
    {
        return 1;
    }
    val  = chan_to_field(red,   &info->var.red);
    val |= chan_to_field(green, &info->var.green);
    val |= chan_to_field(blue,  &info->var.blue);
    pseudo_palette[regno] = val;
    return 0;
}

static struct fb_ops myfb_ops = {
    .owner      = THIS_MODULE,
    .fb_write       = myfb_write,
    .fb_setcolreg   = myfb_setcolreg,       /*设置颜色寄存器*/
    .fb_fillrect    = myfb_fillrect,        /*用像素行填充矩形框，通用库函数*/
    .fb_copyarea    = myfb_copyarea,         /*将屏幕的一个矩形区域复制到另一个区域，通用库函数*/
    .fb_imageblit   = myfb_imageblit,        /*显示一副图像，通用库函数*/
};

static void myfb_update(struct fb_info *fbi, struct list_head *pagelist)
{
    //比较粗暴的方式，直接全部刷新
    fbi->fbops->fb_pan_display(&fbi->var,fbi);  //将应用层数据刷到FrameBuffer缓存中
	fb_refresh(fbi,fbspi);
    printk(KERN_ERR"update function!\n");
}

static struct fb_deferred_io myfb_defio = {
    .delay           = HZ/30,
    .deferred_io     = &myfb_update,
};

static struct fb_var_screeninfo myfb_var = {
    .rotate =         0,
    .xres =           240,
    .yres =           240,
    .xres_virtual =   240,
    .yres_virtual =   240,
    .bits_per_pixel = 16,
    .nonstd =         1,
    /* RGB565 */
    .red.offset =     0,
    .red.length =     5,
    .green.offset =   5,
    .green.length =   6,
    .blue.offset =    11,
    .blue.length =    5,
    .transp.offset =  0,
    .transp.length =  0,
    .activate       = FB_ACTIVATE_NOW,
    .vmode          = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo myfb_fix = {
    .type           = FB_TYPE_PACKED_PIXELS,
    .visual         = FB_VISUAL_TRUECOLOR,
    .line_length    = 240*2,//16 bit = 2byte
    .accel          = FB_ACCEL_NONE,//没有使用硬件加速
    .id             = "myfb",
};

static int myfb_probe(struct spi_device *spi)
{
    int ret;
    void *gmem_addr;
    u32  gmem_size;
    fbspi = spi;  //保存spi驱动指针
    printk(KERN_ERR"register myfb_spi_probe!\n");

    dc_pin = devm_gpiod_get(&spi->dev,"dc",GPIOF_OUT_INIT_LOW);
    if(IS_ERR(dc_pin))
    {
        printk(KERN_ERR"fail to request dc-gpios!\n");
        return -1;
    }
    reset_pin = devm_gpiod_get(&spi->dev,"reset",GPIOF_OUT_INIT_HIGH);
    if(IS_ERR(reset_pin))
    {
        printk(KERN_ERR"fail to request reset-gpios!\n");
        return -1;
    }
    gpiod_direction_output(dc_pin,0);
    gpiod_direction_output(reset_pin,1);

    printk(KERN_INFO"register myfb_probe dev!\n");
    myfb = framebuffer_alloc(sizeof(struct fb_info), &spi->dev);  //向内核申请fb_info结构体
    //初始化底层操作结构体
    myfb->fbops = &myfb_ops;
    gmem_size = 240 * 240 * 2;
    gmem_addr = kmalloc(gmem_size,GFP_KERNEL);  //分配Frame Buffer显存
    if(!gmem_addr)
    {
        printk(KERN_ERR"fail to alloc fb buffer!\n");
    }
    myfb->pseudo_palette = pseudo_palette;
    myfb->var = myfb_var;
    myfb->fix = myfb_fix;
    myfb->screen_buffer = gmem_addr;
    myfb->screen_size = gmem_size;

    myfb->fix.smem_len = gmem_size;          
    myfb->fix.smem_start = (u32)gmem_addr;   
    
    memset((void *)myfb->fix.smem_start, 0, myfb->fix.smem_len); //清楚显示缓存
    

    myfb->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

    myfb->fbdefio = &myfb_defio;

    fb_deferred_io_init(myfb);
    ret = register_framebuffer(myfb);
    if(ret)
    {
        framebuffer_release(myfb);
        unregister_framebuffer(myfb);
        devm_gpiod_put(&spi->dev, dc_pin);
        devm_gpiod_put(&spi->dev, reset_pin);
        printk(KERN_ERR"fail to register fb dev!\n");
        return -1;
    }
    myfb_init(spi);
    return 0;
}

int myfb_remove(struct spi_device *spi)
{
    fb_deferred_io_cleanup(myfb); //清除刷新机制
    unregister_framebuffer(myfb);
    devm_gpiod_put(&spi->dev, dc_pin);
    devm_gpiod_put(&spi->dev, reset_pin);
    return 0;
}


struct of_device_id myfb_match[] = {
    {.compatible = "test,myfb-spi"}, 
    {},
};

struct spi_driver myfb_drv = {
    .probe  = myfb_probe,
    .remove = myfb_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "myfb_spi_driver",
        .of_match_table = myfb_match,
    },
};

module_spi_driver(myfb_drv);

MODULE_LICENSE("GPL");                  //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");                  //版本
MODULE_DESCRIPTION("myfb_spi_driver");  //简单的描述