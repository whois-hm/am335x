/*
	program for  testing am335x_gpio driver 
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "am335x_gpio.h"
int main(void)
{

	int fd = open("/dev/am335x_gpio", O_RDWR);
	if(fd < 0)
	{
		printf("gpio open fail %d %d\n", fd, errno);
		return 0;
	}
	while( 1 )
	{		
		int a, b, c;
		struct am335x_gpio_request r;
		printf("select pin 0~5\n");
		scanf("%d", &a);
		if(a < (int)am335x_gpio_output_pin_0 || 
			a > (int)am335x_gpio_output_pin_5)
		{
			printf("err : invalid pin request\n");
			continue;
		}
		printf("select direction 0 is high 1 is low\n");
		scanf("%d", &b);
		if(b < am335x_gpio_direction_high || 
			b > am335x_gpio_direction_low)
		{
			printf("err : invalid dir request\n");
			continue;
		}

		r.pin = (enum am335x_gpio_output_pin) a;
		r.dir = (enum am335x_gpio_direction) b;
		write(fd, &r, 1);

	}
	close(fd);
	return 0;
	
	
}





