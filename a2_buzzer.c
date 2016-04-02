/* This buzzer driver is based on the buzzer.c file given from blackboard

 * The variable names with ZL or zili are not changed for stablility

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



#define TCFG0	0xE2500000 //T_Out registers addr

#define TCFG1	0xE2500004 

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

 

void buzzer_stop(void) //stop the buzzer   

{  

    unsigned int data;  

    data = readl(ZL_GPD0_CON_ADDR); //  

    data&= ~0xF; // set output mode to 0000

    data|= 0x1;  // set output mode to 0001 for simple output

    writel(data, ZL_GPD0_CON_ADDR); 

    data = readl(ZL_GPD0_CON_DATA);  

    data&=~0x1; // set data as low (off)

    writel(data, ZL_GPD0_CON_DATA);
    
    /*Use tcon to stop the buzzer, but sometime doesn't work
    tcon = readl(ZL_TCON);
    tcon &= ~0x1;	//timer off
    writel(tcon, ZL_TCON);
    */

}  



static void set_freq(unsigned long frequence) {

	unsigned long freq;

	unsigned long data;

	clk_p = clk_get(NULL, "pclk");

	pclk = clk_get_rate(clk_p);

	
	/*Change buzzer output mode*/

	data = readl(ZL_GPD0_CON_ADDR);	

    	data = data&~0xF; //Clear last 4 bits to 0000

    	data = data|0x02; //set outputmode tout_0 - 0010

    	writel(data, ZL_GPD0_CON_ADDR);	
    			   	  
    	/*Change Prescaler0 to 65*/

	tcfg0 = readl(ZL_TCFG0);//

	tcfg0 &= ~0xFF; // Clear last 8 bits to 00000000

	writel(tcfg0, ZL_TCFG0);

	tcfg0 |= 0x41;  // 4*16+1 = 64+1 =65	

	writel(tcfg0, ZL_TCFG0);

	
	/*Set MUX 1/16*/

	tcfg1 = readl(ZL_TCFG1); //

	tcfg1 &= ~0xF;  // Clear last 4 bits to 0000

	writel(tcfg0, ZL_TCFG1);

	tcfg1 |= 0x4;  // 0100 for MUX1/16

	writel(tcfg1, ZL_TCFG1);
	

	freq=pclk/(0x41+1)/0x10;//0x41 for 65

	//freq=(1/frequence)/(1/freq)

	freq=freq/frequence;

	

	tcntb0 = readl(ZL_TCNTB0);

	tcntb0 &= ~0xFFFFFFFF;   // Clear the data to 0

	writel(tcntb0, ZL_TCNTB0);

	tcntb0 |= freq;  	// put freq value to tcntb0

	writel(tcntb0, ZL_TCNTB0);

	

	tcmpb0 = readl(ZL_TCMPB0);

	tcmpb0 &= ~0xFFFFFFFF;  // Clear the data to 0

	writel(tcmpb0, ZL_TCMPB0);

	tcmpb0 = tcntb0/2;	// put freq/2 value to tcmpb0

	writel(tcmpb0, ZL_TCMPB0);

			

	tcon = readl(ZL_TCON);

	tcon &= ~0x1F;   // Clear last 5 bits for register

	writel(tcon, ZL_TCON);

	tcon |= 0x8;	//auto-reload on

	tcon &= ~0x10;	//deadzone off

	tcon &= ~0x4;	//interval off, 

	tcon |= 0x1;	//timer on

	writel(tcon, ZL_TCON);	//write the setting of tcon 

	tcon &= ~0x2;	//manual update off

	writel(tcon, ZL_TCON);	//write the setting of tcon again			

}



static int zili_demo_char_buzzer_open(struct inode *inode, struct file *file) {

	//map the IO physical memory address to virtual address

	ZL_GPD0_CON_ADDR = ioremap(GPD0_CON_ADDR, 0x00000004);

	ZL_GPD0_CON_DATA = ioremap(GPD0_CON_DATA, 0x00000004);
	/*IO remap*/

	ZL_TCFG0 = ioremap(TCFG0, 0x00000004);

	ZL_TCFG1 = ioremap(TCFG1, 0x00000004);

	ZL_TCNTB0 = ioremap(TCNTB0, 0x00000004);

	ZL_TCMPB0 = ioremap(TCMPB0, 0x00000004);

	ZL_TCON = ioremap(TCON, 0x00000004);
	 	
	buzzer_stop();

	printk("Device " DEVICE_NAME " open.\n");

	return 0;

}



static int zili_demo_char_buzzer_close(struct inode *inode, struct file *file) {

	buzzer_stop();

	return 0;

}

static ssize_t zili_demo_char_buzzer_write(struct file *fp, const char *buf, size_t count, loff_t *position){

	unsigned long buzzer_status;

	int  ret;

	ret = copy_from_user(&buzzer_status, buf, sizeof(buzzer_status) );

	if(ret){

		printk("Fail to copy data from the user space to the kernel space!\n");

	}

	if( buzzer_status > 0 ){

		printk("Write Buzzuer Status: %lu\n",buzzer_status);

		set_freq(buzzer_status);

		

	}

	else{

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

	if(ret < 0){

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

	if(ret){

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