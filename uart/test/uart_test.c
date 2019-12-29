/*
* program for uart serial
*/
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  		
#include <fcntl.h>  
#include <unistd.h>  
#include <linux/fb.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>
#include <time.h>
#include <termios.h>
#include <sys/time.h>
int fd = -1; 




unsigned get_tick()
{
	struct timeval v;
	gettimeofday(&v, NULL);
	return (v.tv_sec * 1000) + (v.tv_usec / 1000);
}
void mode_uart_rx(unsigned timeout)
{	
	int i;
	int readsize = 0;
	unsigned char buffer [1024];
	unsigned char *ptr = buffer;
	memset(buffer, 0, 1024);
	unsigned start_tick = get_tick();
	
	while(1)
	{
		int r = read(fd, ptr + readsize, 1024 - readsize);
		if(r > 0)
		{
			readsize += r;
			printf("read [%d / %d]\n", r, readsize);
			printf("data\n");
			for(i = 0; i < readsize; i++)
			{
				printf("%02x ", ptr[i]);
			}
			printf("\n");
		}
		if(readsize >= 1024)
		{
			printf("read over buffersize\n");
			break;
		}
		if(get_tick() - start_tick >= timeout)
		{
			printf("read over timedout\n");
			break;
		}
	}
}
void close_uart()
{
	if(fd != -1)
	{
		close(fd);
	}
}
int open_uart()
{
	int databit;
	int v = 0;
	struct termios t;
	memset(&t, 0, sizeof(struct termios));
	do
	{
		fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY);
		if(fd == -1)
		{
			printf("can't open uart\n");
			break;
		}
		printf("select baudrate\n");
		scanf("%d", &v);
		if(v == 19200) t.c_cflag |= B19200;
		else if(v == 115200) t.c_cflag |= B115200;
		else if(v == 9600) t.c_cflag |= B9600;
		else if(v == 4800) t.c_cflag |= B4800;
		else if(v == 3906) t.c_cflag |= B4800;
		else
		{
			printf("invalid baudrate\n");
			break;
		}

		printf("select databit\n");
		scanf("%d", &v);

		databit = v;
		if(v == 5) t.c_cflag |= CS5;
		else if(v == 6) t.c_cflag |= CS6;
		else if(v == 7) t.c_cflag |= CS7;
		else if(v == 8) t.c_cflag |= CS8;
		else if(v == 9)
		{
			t.c_cflag |= PARENB | CMSPAR;
			t.c_cflag &= ~PARODD;
			t.c_cflag |= CS8;
		}
		else
		{
			printf("invalid databit\n");
			break;
		}

		printf("select stopbit\n");
		scanf("%d", &v);
		if(v == 2) t.c_cflag |= CSTOPB;

		t.c_cflag += CLOCAL | CREAD;

		printf("select parity\n");
		scanf("%d", &v);
		if(v == 0 ||
			v == 1 ||
			v == 2)
		{
 			if(!v && databit != 9) t.c_iflag =IGNPAR;
			if(v == 1)	t.c_cflag |= PARODD | PARENB;
			if(v == 2)	t.c_cflag |= PARENB;
		}
		else
		{
			printf("invalid parity\n");
			break;
		}

		t.c_lflag = 0;
		t.c_oflag = 0;

		t.c_cc[VTIME] = 0.1;
		t.c_cc[VMIN] = 0;

		if(tcflush(fd, TCIFLUSH) == -1)
		{
			printf("can't TCIFLUSH fd\n");
			break;
		}
		if(tcsetattr(fd, TCSANOW, &t) == -1)
		{
			printf("can't TCSANOW fd\n");
			break;
		}	
		return 0;
	}while(0);
	close_uart();
	return -1;
}
int main( int argc, char* argv[] )	
{  
	if(!open_uart())
	{
		while(1)
		{		
			int v;
			printf("select mode : [0 : recv]\n");
			scanf("%d", &v);
			if(v != 0)
			{
				printf("invalid mode select\n");
				continue;
			}
			printf("select read timeout ms\n");
			scanf("%d", &v);	
			mode_uart_rx(v);	
		}
	}

	close_uart();
	return 0;
}  

