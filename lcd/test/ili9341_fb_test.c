/*
*program for ili9341 driver test
* 240 320 16bpp
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

int fd = -1; 
int width = -1;
int height = -1;
int bpp = -1;
int xoffset = -1;
int yoffset = -1;
int lcdsize = -1;
unsigned char *lcd_buffer = NULL;

struct rgb24
{
	unsigned char r, g, b;
};
unsigned short makepixel_24bpp_to16bpp(struct rgb24 color)
{
	return (unsigned short)(((color.r>>3)<<11)|((color.g>>2)<<5)|(color.b>>3));
}
void full_drawing(struct rgb24 color)
{
	int x,y;
	unsigned short *buffer = (unsigned short *)lcd_buffer;

	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{

			*buffer++ = makepixel_24bpp_to16bpp(color);
		}
	}
}
void vertical_drawing(struct rgb24 color_1, 
	struct rgb24 color_2, 
	struct rgb24 color_3,
	struct rgb24 color_4)
{
	int x,y;
	unsigned short *buffer = (unsigned short *)lcd_buffer;

	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			if(x < 80)
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_1);
			}
			else if(x >= 80 && x < 160)
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_2);
			}
			else if(x >= 160 && x < 240)
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_3);
			}
			else
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_4);
			}
		}
	}	
}
void horizon_drawing(struct rgb24 color_1, 
	struct rgb24 color_2, 
	struct rgb24 color_3)
{
	int x,y;
	unsigned short *buffer = (unsigned short *)lcd_buffer;

	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			if(y < 80)
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_1);
			}
			else if(y >= 80 && y < 160)
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_2);
			}
			else
			{
				*buffer++ = makepixel_24bpp_to16bpp(color_3);
			}
		}
	}	

}
struct rgb24 get_rand_rgb()
{
	struct rgb24 a;
	a.r = (rand() % 256);
	a.g = (rand() % 256);
	a.b = (rand() % 256);
	return a;
}
int get_rand_position(int last)
{
	return (rand() % (last+1));
}
void randdot_drawing()
{

	unsigned short *buffer = (unsigned short *)lcd_buffer;
	int max_count = (width * height) / 2;
	while(max_count-- )
	{
		int x = get_rand_position(width);
		int y = get_rand_position(height);
		buffer[(y * width) + x] = makepixel_24bpp_to16bpp(get_rand_rgb());
		usleep(10 * 1000);
	}
}
void close_lcd()
{
	if(lcd_buffer)
	{
		munmap( lcd_buffer, lcdsize );	 
	}
	if(fd != -1)
	{
		close(fd);
	}
	lcd_buffer = NULL;
	fd  = -1;
}
int open_lcd()
{
 
	struct fb_var_screeninfo framebuffer_variable_screeninfo;  
	struct fb_fix_screeninfo framebuffer_fixed_screeninfo;	
	
	do
	{
		fd = open("/dev/fb0", O_RDWR);
		if(fd < 0)
		{
			printf("can't open framebuffer\n");
			break;
		}
		if( ioctl(fd, FBIOGET_VSCREENINFO,   
			&framebuffer_variable_screeninfo) )
		{
			printf("can't get buffer swinfo\n");
			break;
		}
		if( ioctl(fd, FBIOPUT_VSCREENINFO,   
			&framebuffer_variable_screeninfo) )
		{
			printf("can't get buffer hwinfo\n");
			break;
		}
		if ( ioctl(fd, FBIOGET_FSCREENINFO,   
			&framebuffer_fixed_screeninfo) )  
		{  
			printf( "can't get buffer fixinfo\n" );  
			break;
		}

		printf( "framebuffer display information\n" );	
		printf( " %d x %d  %d bpp\n", framebuffer_variable_screeninfo.xres,  
			 framebuffer_variable_screeninfo.yres,	 
			 framebuffer_variable_screeninfo.bits_per_pixel );	

		width  = framebuffer_variable_screeninfo.xres;	
		height = framebuffer_variable_screeninfo.yres;	
		bpp = framebuffer_variable_screeninfo.bits_per_pixel/8;  
		xoffset = framebuffer_variable_screeninfo.xoffset;	
		yoffset = framebuffer_variable_screeninfo.yoffset;	
		lcdsize = width * height * bpp;
		
		lcd_buffer = (unsigned char*)mmap( 0, lcdsize,  
			PROT_READ|PROT_WRITE,  
			MAP_SHARED,  
			fd, 0 );  
											
		if ( lcd_buffer == MAP_FAILED ) 
		{
			printf("can't get mmap\n");
			break;
		}
		return 0;
	}while(0);

	close_lcd();
	return -1;
}
		

int main( int argc, char* argv[] )	
{  
	do
	{	
		int c = 5;
		struct rgb24 black = {0,0,0};
		srand(time(NULL));
		if(open_lcd() < 0)
		{
			break;
		}
		while(c--)
		{
			full_drawing(get_rand_rgb());
			usleep(3000 * 1000);
		}
		
		c = 5;
		while(c--)
		{
			vertical_drawing(get_rand_rgb(), 
				get_rand_rgb(), 
				get_rand_rgb(), 
				get_rand_rgb());
			usleep(3000 * 1000);
		}
		c = 5;
		while(c--)
		{
			horizon_drawing(get_rand_rgb(), 
				get_rand_rgb(), 
				get_rand_rgb());
			usleep(3000 * 1000);
		}
		full_drawing(black);
		randdot_drawing();
		full_drawing(black);
		
	}while(0);

 close_lcd();
}  

