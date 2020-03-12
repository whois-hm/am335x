
enum am335x_gpio_output_pin
{
	am335x_gpio_output_pin_0 = 0,
	am335x_gpio_output_pin_1,
	am335x_gpio_output_pin_2,
	am335x_gpio_output_pin_3,
	am335x_gpio_output_pin_4,
	am335x_gpio_output_pin_5,
	am335x_gpio_output_pin_max
};
enum am335x_gpio_direction
{
	am335x_gpio_direction_high = 0,
	am335x_gpio_direction_low,
	am335x_gpio_direction_max
};

struct am335x_gpio_request
{
	enum am335x_gpio_output_pin pin;
	enum am335x_gpio_direction dir;
};


#define AM335X_GPIO_IOCTL_MAGIC 'A'
#define AM335X_GPIO_IOCTL_REQ_PWM	0
#define AM335X_GPIO_IOCTL_REQ_MAX	1
#define AM335X_GPIO_IOCTL_RQ_PWM_PAR	unsigned

#define AM335X_REQ_PWM _IOR(AM335X_GPIO_IOCTL_MAGIC, AM335X_GPIO_IOCTL_REQ_PWM, AM335X_GPIO_IOCTL_RQ_PWM_PAR)