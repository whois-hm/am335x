#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include "am335x_gpio.h"

const char  *char_dev_name = "am335x_gpio";
int char_dev_name_major = 125;
struct am335x_gpio_request *from_user_buffer = NULL;
int from_user_max = 10;




/*
*output pin description
*pin 12 : PROC(T12) / NAME(GPIO1_12) / conf_gpmc_ad12 / gpio1[12] / am335x_gpio_pin_0                      
*pin 14 : PROC(T11) / NAME(GPIO0_26) / conf_gpmc_ad10 / gpio0[26] / am335x_gpio_pin_1
*pin 16 : PROC(V13) / NAME(GPIO1_14) / conf_gpmc_ad14 / gpio1[14] / am335x_gpio_pin_2
*pin 18 : PROC(V12) / NAME(GPIO2_1) / conf_gpmc_clk_mux0 / gpio2[1] / am335x_gpio_pin_3
*pin 20 : PROC(V9) / NAME(GPIO1_31) / conf_gpmc_csn2 / gpio1[31] / am335x_gpio_pin_4
*pin 22 : PROC(V8) / NAME(GPIO1_5) / conf_gpmc_ad5 / gpio1[5] / am335x_gpio_pin_5
*/


/*
*gpio bank registers
*/
#define GPIO_SIZE				4096
#define GPIO0_START_ADDRESS		0x44E07000
#define GPIO0_END_ADDRESS		0x44E07FFF
#define GPIO1_START_ADDRESS		0x4804C000
#define GPIO1_END_ADDRESS		0x4804CFFF
#define GPIO2_START_ADDRESS		0x481AC000
#define GPIO2_END_ADDRESS		0x481ACFFF
#define GPIO_OE_REGISTER		0x00000134


#define GPIO_OUTPUT_REGISTER	0x0000013C
#define GPIO_CLEAR_REGISTER		0x00000190
#define GPIO_SET_REGISTER		0x00000194




/*
*gpio clock enable register
*/
#define CM_WKUP_REGISTER		0x44E00400
#define CM_WKUP_REGISTER_SIZE	256
#define CM_WKU_GPIO0_CLKCTRL	0x00000008
#define CM_PER_REGISTER			0x44E00000
#define CM_PER_RESITER_SIZE		4096
#define CM_PER_GPIO1_CLKCTRL	0x000000AC
#define CM_PER_GPIO2_CLKCTRL	0x000000B0
#define CM_CLK_ENABLE			0x00000002

/*
*gpio mode setup register
*/
#define CONTROL_MODULE_REGISTER	0x44E10000
#define CONF_GPMC_SIZE			8192
#define CONF_GPMC_AD12			0x00000830
#define CONF_GPMC_AD10			0x00000828
#define CONF_GPMC_AD14			0x00000838
#define CONF_GPMC_CLK_MUX0		0x0000088C
#define CONF_GPMC_CSN2			0x00000884
#define CONF_GPMC_AD5			0x00000814



typedef unsigned char * HWREG;	

HWREG gpio0_register_ptr = NULL;
HWREG gpio1_register_ptr = NULL;
HWREG gpio2_register_ptr = NULL;

HWREG control_module_register_ptr_t = NULL;
HWREG clock_enable_register_ptr_t_0 = NULL;
HWREG clock_enable_register_ptr_t_12= NULL;

struct
{
	HWREG t_reg;
	int pin;
}static cotrol_table[am335x_gpio_output_pin_max] =
{	
		{NULL, -1}, 
		{NULL, -1}, 
		{NULL, -1}, 
		{NULL, -1}, 
		{NULL, -1}, 
		{NULL, -1}, 
};

static void set_output_register(HWREG reg, int pin, int high);
static void get_ouput_register(struct am335x_gpio_request *r, int i);

int am335x_gpio_driver_open(struct inode *inode, struct file *filp)
{
	return 0;
}
int am335x_gpio_driver_close(struct inode *inode, struct file *filp)
{
	return 0;
}
ssize_t am335x_gpio_driver_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	struct am335x_gpio_request *from_user = NULL;

	if(count <= 0 ||
		count > from_user_max)
	{
		return -EINVAL;
	}
	
	from_user = (struct am335x_gpio_request *)buf;

	while(count--)
	{
		get_ouput_register(from_user + count, (int)count);	
	}
	if(copy_to_user(buf, from_user_buffer, count * sizeof(struct am335x_gpio_request) ) != 0)
	{
		return -EFAULT;
	}
	return count;

}

ssize_t am335x_gpio_driver_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{


	if(count <= 0 ||
		count > from_user_max)
	{
		return -EINVAL;
	}
	
	if(copy_from_user(from_user_buffer, buf, count * sizeof(struct am335x_gpio_request)) != 0)
	{
		return -EFAULT;
	}


	while(count--)
	{
		set_output_register(cotrol_table[(int)from_user_buffer[count].pin].t_reg, 
			cotrol_table[(int)from_user_buffer[count].pin].pin,
			from_user_buffer[count].dir == am335x_gpio_direction_high ? 1 : 0);
	}
	return count;
}

static void set_output_register(HWREG reg, int pin, int high)
{
	printk(KERN_INFO "set_register : %dpin %s\n", pin, high ? "high" : "low");

	if(high)
	{
		*((unsigned  *)(reg + GPIO_SET_REGISTER)) |= (1 << pin);
		return;
	}
	*((unsigned  *)(reg + GPIO_CLEAR_REGISTER)) |= (1 << pin);	
}
static void get_ouput_register(struct am335x_gpio_request *r, int i)
{
	from_user_buffer[i].pin = r->pin;
	from_user_buffer[i].dir = (*((unsigned *)(cotrol_table[(int)r->pin].t_reg + GPIO_OUTPUT_REGISTER)) & (1 << cotrol_table[(int)r->pin].pin)) ? 
		am335x_gpio_direction_high : am335x_gpio_direction_low;
}

static void gpio_register_setup(void )
{
	gpio0_register_ptr = ioremap(GPIO0_START_ADDRESS, GPIO_SIZE);
	gpio1_register_ptr = ioremap(GPIO1_START_ADDRESS, GPIO_SIZE);
	gpio2_register_ptr = ioremap(GPIO2_START_ADDRESS, GPIO_SIZE);
}
static void gpio_register_cleanup(void)
{
 	if(gpio0_register_ptr) iounmap(gpio0_register_ptr);
	if(gpio1_register_ptr) iounmap(gpio1_register_ptr);
	if(gpio2_register_ptr) iounmap(gpio2_register_ptr);	 
	gpio0_register_ptr = NULL;
	gpio1_register_ptr = NULL;
	gpio2_register_ptr = NULL;
}
static void gpio_register_set_oe(void )
{
	volatile unsigned *register_ptr = NULL;

	register_ptr = (volatile unsigned *)((gpio0_register_ptr) + GPIO_OE_REGISTER);
	*register_ptr &= 0xFBFFFFFF;

	register_ptr = (volatile unsigned *)((gpio1_register_ptr) + GPIO_OE_REGISTER);
	*register_ptr &= 0x7FFFAFDF;

	register_ptr = (volatile unsigned *)((gpio2_register_ptr) + GPIO_OE_REGISTER);
	*register_ptr &= 0xFFFFFFFD;
}
static void clock_enable_setup(void)
{
	volatile unsigned *register_ptr = NULL;

	clock_enable_register_ptr_t_0 =  ioremap(CM_WKUP_REGISTER, CM_WKUP_REGISTER_SIZE);
	clock_enable_register_ptr_t_12 =  ioremap(CM_PER_REGISTER, CM_PER_RESITER_SIZE);
	if(!clock_enable_register_ptr_t_0 || 
		!clock_enable_register_ptr_t_12)
	{
		printk(KERN_INFO "can't clock_enable_setup");
		return ;
	}

	register_ptr =  (volatile unsigned  *)(clock_enable_register_ptr_t_0 + CM_WKU_GPIO0_CLKCTRL);
	*register_ptr = CM_CLK_ENABLE;
	register_ptr =  (volatile unsigned  *)(clock_enable_register_ptr_t_12 + CM_PER_GPIO1_CLKCTRL);
	*register_ptr = CM_CLK_ENABLE;
	register_ptr =  (volatile unsigned  *)(clock_enable_register_ptr_t_12 + CM_PER_GPIO2_CLKCTRL);
	*register_ptr = CM_CLK_ENABLE;

}
static void clock_enable_cleanup(void)
{
	if(clock_enable_register_ptr_t_0) iounmap(clock_enable_register_ptr_t_0);
	if(clock_enable_register_ptr_t_12) iounmap(clock_enable_register_ptr_t_12);	 

	clock_enable_register_ptr_t_0 = NULL;
	clock_enable_register_ptr_t_12 = NULL;

}
static void control_module_setup(void)
{
	volatile unsigned *register_ptr = NULL;

	control_module_register_ptr_t =  ioremap(CONTROL_MODULE_REGISTER, CONF_GPMC_SIZE);
	if(!control_module_register_ptr_t)
	{
		printk(KERN_INFO "can't control_module_setup\n");
		return;
	}

	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_AD12);
	*register_ptr |= 0x00000007; 
	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_AD10);
	*register_ptr |= 0x00000007; 
	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_AD14);
	*register_ptr |= 0x00000007; 
	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_CLK_MUX0);
	*register_ptr |= 0x00000007; 
	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_CSN2);
	*register_ptr |= 0x00000007; 
	register_ptr = (volatile unsigned *)(control_module_register_ptr_t + CONF_GPMC_AD5);
	*register_ptr |= 0x00000007; 
	

}
static void control_module_cleanup(void)
{
	if(control_module_register_ptr_t)
	{
		iounmap(control_module_register_ptr_t);
		control_module_register_ptr_t = NULL;
	}
}
static void register_map_setup(void)
{
	from_user_buffer = kmalloc(sizeof(struct am335x_gpio_request) * from_user_max, GFP_KERNEL);
	cotrol_table[am335x_gpio_output_pin_0].t_reg = gpio1_register_ptr;
	cotrol_table[am335x_gpio_output_pin_0].pin = 12;

	cotrol_table[am335x_gpio_output_pin_1].t_reg = gpio0_register_ptr;
	cotrol_table[am335x_gpio_output_pin_1].pin = 26;

	cotrol_table[am335x_gpio_output_pin_2].t_reg = gpio1_register_ptr;
	cotrol_table[am335x_gpio_output_pin_2].pin = 14;

	cotrol_table[am335x_gpio_output_pin_3].t_reg = gpio2_register_ptr;
	cotrol_table[am335x_gpio_output_pin_3].pin = 1;

	cotrol_table[am335x_gpio_output_pin_4].t_reg = gpio1_register_ptr;
	cotrol_table[am335x_gpio_output_pin_4].pin = 31;

	cotrol_table[am335x_gpio_output_pin_5].t_reg = gpio1_register_ptr;
	cotrol_table[am335x_gpio_output_pin_5].pin = 5;

}
static void register_map_cleanup(void)
{
	kfree(from_user_buffer);	
}

static struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.open= am335x_gpio_driver_open,
	.release= am335x_gpio_driver_close,
	.read= am335x_gpio_driver_read,
	.write= am335x_gpio_driver_write
};
static void am335x_gpio_exit( void )
{
	register_map_cleanup();
	clock_enable_cleanup();
	control_module_cleanup();
	gpio_register_cleanup();
	
	unregister_chrdev(char_dev_name_major, char_dev_name);

	printk(KERN_INFO "unload module gpio\n");
}

static int am335x_gpio_init( void )
{
	int ret = 0;

	ret = register_chrdev(char_dev_name_major, char_dev_name, &fops);
	if(	ret < 0)
	{
		printk(KERN_WARNING "can't module init gpio %d\n", ret);
		return ret;
	}

	control_module_setup();
	clock_enable_setup();
	gpio_register_setup();
	gpio_register_set_oe();
	register_map_setup();

	return 0;
}

module_init(am335x_gpio_init);
module_exit(am335x_gpio_exit);
MODULE_LICENSE("GPL");


