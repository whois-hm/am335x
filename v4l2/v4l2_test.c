/*
*program for v412 camera test
* C170 has fourcc YUV422 MJPEG
* We use yuv422 format test only 
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
#include <linux/input.h>
#include <linux/videodev2.h>
#include <sys/shm.h>
#include <sys/poll.h>
#include <errno.h>

#define FOURCC(a,b,c,d)\
	((unsigned)(a) | ((unsigned)(b) << 8) | ((unsigned)(c) << 16) | ((unsigned)(d) << 24))

int fd = -1;
int v4l2_io_mode = -1;/*0 mean streaming mode, 1 mean rwmode*/
unsigned width;
unsigned height;
unsigned byteper_line;
unsigned byteper_frame;
int buffercount ;
struct frame
{
	unsigned size;
	void *ptr;
};
struct frame *frames = NULL;
int xioctl(int fd, int request, void *arg)
{
	int r;
	do{
		r = ioctl(fd, request, arg);
	}while(-1 == r && EINTR == errno);
	return r;
}
int open_v412()
{
	unsigned ourYUYVfourcc = FOURCC('Y', 'U', 'Y', 'V');
	struct v4l2_capability cap;
	struct v4l2_fmtdesc desc;
	struct v4l2_format format;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;
	unsigned n = 0;
	unsigned indx = 0;
	unsigned t = 0;
	unsigned bfmt = 0;

	memset(&cap, 0, sizeof(struct v4l2_capability));
	memset(&desc, 0, sizeof(struct v4l2_fmtdesc));
	memset(&format, 0, sizeof(struct v4l2_format));
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	memset(&buf, 0, sizeof(struct v4l2_buffer));
	


	int i = 0;
	desc.index= indx;
	desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	
	fd = open("/dev/video0", O_RDWR|O_NONBLOCK, 0);

	if(fd < 0)
	{
		printf("can't get v412 fd\n");
		return -1;
	}
	if(xioctl(fd, VIDIOC_QUERYCAP, &cap))
	{
		printf("can't get v412 capability\n");
		close(fd);
		fd = -1;
		return -1;
	}
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		printf("v4l2 no video\n");
		close(fd);
		fd = -1;
		return -1;		
	}
	
	if(cap.capabilities & V4L2_CAP_STREAMING)	 v4l2_io_mode = 0;
	else if(cap.capabilities & V4L2_CAP_READWRITE)	 v4l2_io_mode = 1;
	else
	{
		printf("v4l2 can't support io mode\n");
		close(fd);
		fd = -1;
		return -1;
	}

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(xioctl(fd, VIDIOC_G_FMT, &format))
	{
		printf("can't get fmt %d %s\n", errno, strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}

	if(format.fmt.pix.pixelformat != ourYUYVfourcc)
	{
		printf("no support fmt\n");
		close(fd);
		fd = -1;
		return -1;
	}
	

	while(!(xioctl(fd, VIDIOC_ENUM_FMT, &desc)))
	{
		if(ourYUYVfourcc == desc.pixelformat)
		{
			bfmt = 1;
			break;
		}
		indx++;
		desc.index = indx;

	}

	if(!bfmt)
	{
		printf("cam no has our fourcc\n");
		close(fd);
		fd = -1;
		return -1;
	}
	width = format.fmt.pix.width;
	height = format.fmt.pix.height;
	byteper_line = format.fmt.pix.bytesperline;
	byteper_frame = format.fmt.pix.sizeimage;	


	if(v4l2_io_mode == 0)
	{
		req.count = 4;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		if((xioctl(fd, VIDIOC_REQBUFS, &req)))
		{
			printf("can't reqbufs\n");
			close(fd);
			fd = -1;
			return -1;
		}
		if(req.count < 2)
		{
			printf("reqbufs too short\n");
			close(fd);
			fd = -1;
			return -1;
		}
		buffercount = req.count;
		frames = (struct frame *)malloc(sizeof(struct frame) * buffercount);
		for(i = 0; i < buffercount; i++)
		{
			memset(&buf, 0, sizeof(struct v4l2_buffer));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			if(xioctl(fd, VIDIOC_QUERYBUF, &buf))
			{
				printf("can't querybuf\n");
				close(fd);
				fd = -1;
				return -1;
			}
			frames[i].size = buf.length;
			frames[i].ptr = mmap(NULL, buf.length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, buf.m.offset);
			if(MAP_FAILED == frames[i].ptr)
			{
				printf("can't querybuf\n");
				close(fd);
				fd = -1;
				return -1;
			}						
		}		

		
			for(i = 0; i < buffercount; i++)
			{
				memset(&buf, 0, sizeof(struct v4l2_buffer));
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;
				if(xioctl(fd, VIDIOC_QBUF, &buf))
				{
					printf("can't qbuf\n");
					close(fd);
					fd = -1;
					return -1;
				}
			}
			
			
			printf("mode at 0\n");
			
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if(xioctl(fd, VIDIOC_STREAMON, &type))
			{
				
				printf("can't stream on\n");
				close(fd);
				fd = -1;
				return -1;
			}
			
	}
	if(v4l2_io_mode == 1)
	{
		printf("mode at 1\n");
		buffercount = 1;
		frames = (struct frame *)malloc(sizeof(struct frame) * buffercount);
		frames->size = byteper_frame;
		frames->ptr = (void *)malloc(byteper_frame);
	}

	return 0;
	
}

void close_v4l2()
{
	if(fd >= 0)
	{
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl(fd, VIDIOC_STREAMOFF, &type);
	}
	if(frames)
	{
		free(frames);	
		frames = NULL;
	}
	if(fd != -1)
	{
		close(fd);
		fd = -1;
	}
}


void read_v4l2(int frame_count)
{
	int a = 0;
	while(1)
	{
		int res = -1;
		struct v4l2_buffer buf;
		unsigned readbyte = 0;
		unsigned remainframe = 0;
		char *pbuf = NULL;


		struct pollfd signalfd;		
		memset(&signalfd, 0, sizeof(struct pollfd));
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		signalfd.fd = fd;
		signalfd.events =(POLLIN | POLLPRI | POLLRDNORM);


		res = poll(&signalfd, 1, -1);

		printf("res = %d\n", res);
		if(res == -1)
		{
			printf("reading fail\n");
			break;
		}
		if(res == 0)
		{
			printf("read_v4l2 res 0\n");
			continue;
		}
		if(signalfd.revents & ( POLLERR | POLLHUP | POLLNVAL))
		{
						printf("poll has err\n");		
						if(signalfd.revents & POLLERR)
							{
													printf("poll has err 0 \n");		
							}
												if(signalfd.revents & POLLHUP)
							{
													printf("poll has err1\n");		
							}
																		if(signalfd.revents & POLLNVAL)
							{
													printf("poll has err2\n");		
							}
		}
		if(signalfd.revents & (POLLIN | POLLPRI | POLLRDNORM))
		{
						printf("reading frame %d\n", a++);
			if(v4l2_io_mode == 0)		
			{
				memset(&buf, 0, sizeof(struct v4l2_buffer));
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				if(xioctl(fd, VIDIOC_DQBUF, &buf))
				{
					printf("reading dequeue fail\n");
					break;
				}
				if(xioctl(fd, VIDIOC_QBUF, &buf))
				{
					printf("reading enqueue fail\n");
					break;
				}
				if(buf.index >= buffercount)
				{
					printf("reading invalid index reading\n");
					break;
				}

			}
			if(v4l2_io_mode == 1)		
			{
				remainframe = frames->size;
				pbuf = (char *)frames->ptr;
				do
				{
					readbyte = read(fd, pbuf + frames->size - remainframe, remainframe);
					if(readbyte < 0)
					{
						if(errno == EIO || errno == EAGAIN)
						{
							continue;
						}
						else
						{
							printf("reading invalid index reading\n");
							goto ioreadfail;
						}
					}
					if(readbyte >= 0)
						remainframe -= readbyte;
					
				}while(remainframe > 0);
				printf("reading frame %d\n", a++);
			}
		}
	}	
ioreadfail:	
	printf("");
	
}



int main()
{
	if(open_v412() >= 0)
	{
		read_v4l2(10);
	}
	close_v4l2();
}
