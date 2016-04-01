/*comp3438_buzzer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-timer.h>
#include <linux/clk.h>


#define DEVICE_NAME				"buzzer"
#define N_D         1                   /*Number of Devices*/
#define S_N         1                   /*The start minor number*/

#define GPD0_CON_ADDR	0xE02000A0 
#define GPD0_CON_DATA	0Xe02000A4

#define TCFG0	0xE2500000 // timer config reg
#define TCFG1	0xE2500004 // timer config reg
#define TCNTB0  0xE250000C
#define TCMPB0  0xE2500010
#define TCON	0xE2500008

static unsigned long tcfg0;
static unsigned long tcfg1;
static unsigned long tcntb0;
static unsigned long tcmpb0;
static unsigned long tcon;

static void *ZL_GPD0_CON_ADDR;
static void *ZL_GPD0_CON_DATA;

static void *ZL_TCFG0;
static void *ZL_TCFG1;
static void *ZL_TCNTB0;
static void *ZL_TCMPB0;
static void *ZL_TCON;

//static struct semaphore lock;

static int          major;
static dev_t        devno;
static struct cdev  dev_buzzer;


unsigned long pclk;
struct clk *clk_p;


void buzzer_start(void) //start the buzzer  
{  
    unsigned int data;  
    data = readl(ZL_GPD0_CON_DATA); //  
    data = data|0x01; // set data as high   
    writel(data, ZL_GPD0_CON_DATA);
}  
  
  
void buzzer_stop(void) //stop the buzzer   
{  
    unsigned int data;  
    data = readl(ZL_GPD0_CON_DATA); //  
    data&=~0x01; // set data as low
    writel(data, ZL_GPD0_CON_DATA); 
}  

static void set_freq(unsigned long frequence) {
	unsigned long freq;
	clk_p = clk_get(NULL, "pclk");
	pclk = clk_get_rate(clk_p);
	
	ZL_TCFG0 = ioremap(TCFG0, 0x00000004);
	ZL_TCFG1 = ioremap(TCFG1, 0x00000004);
	ZL_TCNTB0 = ioremap(TCNTB0, 0x00000004);
	ZL_TCMPB0 = ioremap(TCMPB0, 0x00000004);
	ZL_TCON = ioremap(TCON, 0x00000004);
	
	tcfg0 = readl(ZL_TCFG0); //
	tcfg0 &= ~0xFFFFFFFF;   // 
	writel(tcfg0, ZL_TCFG0);
	printk("asdd: %lu\n",tcfg0);
	tcfg0 |= 0x41;  // 	
	writel(tcfg0, ZL_TCFG0);
	printk("%lu\n",tcfg0);
	
	tcfg1 = readl(ZL_TCFG1); //
	tcfg1 &= ~0xFFFFFFFF;   // 
	writel(tcfg0, ZL_TCFG1);
	printk("asdd1: %lu\n",tcfg1);
	tcfg1 |= 0x10;  // 	
	writel(tcfg1, ZL_TCFG1);
	printk("%lu \n",tcfg1);
		
	freq=pclk/(tcfg0+1)/tcfg1;
	freq=(1/frequence)/(1/freq);
	
	tcntb0 = readl(ZL_TCNTB0);
	tcntb0 &= ~0xFFFFFFFF;   // 
	writel(tcntb0, ZL_TCNTB0);
	printk("tcntb0: %lu\n",tcntb0);
	tcntb0 |= freq;  // 	
	writel(tcntb0, ZL_TCNTB0);
	printk("%lu \n",tcntb0);
	
	tcmpb0 = readl(ZL_TCMPB0);
	tcmpb0 &= ~0xFFFFFFFF;   // 
	writel(tcmpb0, ZL_TCMPB0);
	printk("tcntb0: %lu\n",tcmpb0);
	tcmpb0 = tcntb0/2;	
	writel(tcmpb0, ZL_TCMPB0);
	printk("%lu \n",tcmpb0);	
		
	tcon = readl(ZL_TCON);
	tcon &= ~0xFFFFFFFF;   // 
	writel(tcon, ZL_TCON);
	printk("tcon: %lu\n",tcon);
	tcon |= 0x8;	//auto-reload on
	tcon &= 0x9;	//interval off, manual update off
	tcon &= 0xEF;	//deadzone off
	writel(tcon, ZL_TCMPB0);
	printk("%lu \n",tcon);	
	
}

static int zili_demo_char_buzzer_open(struct inode *inode, struct file *file) {
	unsigned int data;
	//map the IO physical memory address to virtual address
	ZL_GPD0_CON_ADDR = ioremap(GPD0_CON_ADDR, 0x00000004);
	ZL_GPD0_CON_DATA = ioremap(GPD0_CON_DATA, 0x00000004);
	//configure the GPIO work as output, set the last four bits of the register as 0010
	data = readl(ZL_GPD0_CON_ADDR);
	data = data&(~0x01<<1);   
   	data = data&(~0x01<<2);   
    	data = data&(~0x01<<3); 	
    	data = data|0x02;  
    	printk("Load Data: %u\n",data);
    	writel(data, ZL_GPD0_CON_ADDR);	

	printk("Device " DEVICE_NAME " open.\n");
	return 0;
}

static int zili_demo_char_buzzer_close(struct inode *inode, struct file *file) {
	unsigned int data;
	ZL_GPD0_CON_ADDR = ioremap(GPD0_CON_ADDR, 0x00000004);
	ZL_GPD0_CON_DATA = ioremap(GPD0_CON_DATA, 0x00000004);
	//configure the GPIO work as output, set the last four bits of the register as 0010
	data = readl(ZL_GPD0_CON_ADDR);
	data = data&(~0x01<<1);   
   	data = data&(~0x01<<2);   
    	data = data&(~0x01<<3); 	
    	data = data|0x01;  
    	printk("Close Data: %u\n",data);
    	writel(data, ZL_GPD0_CON_ADDR);	
    	writel(0, ZL_GPD0_CON_DATA);
	return 0;
}

static ssize_t zili_demo_char_buzzer_write(struct file *fp, const char *buf, size_t count, loff_t *position)
{
	char buzzer_status;
	int  ret;
	ret = copy_from_user(&buzzer_status, buf, sizeof(buzzer_status) );
	if(ret)
	{
		printk("Fail to copy data from the user space to the kernel space!\n");
	}
	if( buzzer_status > 0 )
	{
		printk("buzzer_status: %d",buzzer_status);
		//buzzer_start();
		set_freq(buzzer_status);
		
	}
	else
	{
		buzzer_stop();
	}
	return sizeof(buzzer_status);
}

static struct file_operations zili_mini210_pwm_ops = {
	.owner			= THIS_MODULE,
	.open			= zili_demo_char_buzzer_open,
	.release		= zili_demo_char_buzzer_close, 
	.write			= zili_demo_char_buzzer_write,
};

static int __init zili_demo_char_buzzer_dev_init(void) {
	int ret;
	/*Register a major number*/
	ret = alloc_chrdev_region(&devno, S_N, N_D, DEVICE_NAME);
	if(ret < 0)
	{
		printk("Device " DEVICE_NAME " cannot get major number.\n");
		return ret;
	}
	
	major = MAJOR(devno);
	printk("Device " DEVICE_NAME " initialized (Major Number -- %d).\n", major);
	
	/*Register a char device*/
	cdev_init(&dev_buzzer, &zili_mini210_pwm_ops);
	dev_buzzer.owner = THIS_MODULE;
	dev_buzzer.ops   = &zili_mini210_pwm_ops;
	ret = cdev_add(&dev_buzzer, devno, N_D);
	if(ret)
	{
		printk("Device " DEVICE_NAME " register fail.\n");
		return ret;
	}
	return 0;
	
}

static void __exit zili_demo_char_buzzer_dev_exit(void) {
	buzzer_stop();
	cdev_del(&dev_buzzer);
	unregister_chrdev_region(devno, N_D);
	printk("Device " DEVICE_NAME " unloaded.\n");
}

module_init(zili_demo_char_buzzer_dev_init);
module_exit(zili_demo_char_buzzer_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");
MODULE_DESCRIPTION("S5PV210 Buzzer Driver");