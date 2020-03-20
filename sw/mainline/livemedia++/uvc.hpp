#pragma once
#if defined _platform_linux
#define have_uvc
class uvc
{
private:
	int uvc_open_test()
	{
		/*
		 	 open driver
		 */

		int fd = -1;
		struct v4l2_capability cap;
		struct v4l2_fmtdesc desc;
		struct v4l2_format format;
		struct v4l2_requestbuffers req;
		struct v4l2_buffer buf;
		int indx = 0;
		int bfmt = 0;

		memset(&cap, 0, sizeof(struct v4l2_capability));
		memset(&desc, 0, sizeof(struct v4l2_fmtdesc));
		memset(&format, 0, sizeof(struct v4l2_format));
		memset(&req, 0, sizeof(struct v4l2_requestbuffers));
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		do
		{
			/*
			 	 usefull pipe when user 'waitframe' in other thread
			 */
			int res = pipe(_pipe);
			if(res < 0)
			{
				goto fail;
			}
			desc.index = indx;
			desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			req.count = 4;
			req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			req.memory = V4L2_MEMORY_MMAP;


			fd = open(_dev, O_RDWR | O_NONBLOCK , 0);
			if(fd < 0)
			{
				goto fail;
			}
			if(xioctl(fd, VIDIOC_QUERYCAP, &cap))
			{
				goto fail;
			}
			if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
			{
				goto fail;
			}
			if(cap.capabilities & V4L2_CAP_STREAMING)	 _video_capture_mode = 0;
			else if(cap.capabilities & V4L2_CAP_READWRITE)	 _video_capture_mode = 1;
			else
			{
				goto fail;
			}

			struct v4l2_crop crop;
			struct v4l2_cropcap cropcap;
			memset(&cropcap, 0, sizeof(struct v4l2_cropcap));
			memset(&crop, 0, sizeof(struct v4l2_crop));

			cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if(xioctl(fd, VIDIOC_CROPCAP, &cropcap))
			{
				crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				crop.c = cropcap.defrect;
				xioctl(fd, VIDIOC_S_CROP, &crop);

			}


			
			if(xioctl(fd, VIDIOC_G_FMT, &format))
			{
				goto fail;
			}

			xioctl(fd, VIDIOC_S_FMT, &format);
			
			if(format.fmt.pix.pixelformat != _support_fourcc)
			{
				goto fail;
			}
			while(!(xioctl(fd, VIDIOC_ENUM_FMT, &desc)))
			{
				if(_support_fourcc == desc.pixelformat)
				{
					bfmt = 1;
					break;
				}
				indx++;
				desc.index = indx;
			}
			if(!bfmt)
			{
				goto fail;
			}


			/*
			 	 mode 0 mean that io mmap mode
			 */
			if(_video_capture_mode == 0)
			{
				if((xioctl(fd, VIDIOC_REQBUFS, &req)))
				{
					goto fail;
				}
				if(req.count < 2)
				{
					goto fail;
				}
				for(int i = 0; i < req.count; i++)
				{
					memset(&buf, 0, sizeof(struct v4l2_buffer));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index = i;
					if(xioctl(fd, VIDIOC_QUERYBUF, &buf))
					{
						goto fail;
					}
					void *ptr = mmap(NULL, buf.length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, buf.m.offset);
					if(MAP_FAILED == ptr)
					{
						goto fail;
					}
					_videoframes.push_back(std::make_pair(ptr, buf.length));
				}
				
					{
											enum v4l2_buf_type type;
					type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					ioctl(_fd, VIDIOC_STREAMOFF, &type);
					}


			}
			/**
			 	 mode 1 mean that mode rw mode
			 */
			if(_video_capture_mode == 1)
			{
				void *ptr = __base__malloc__(format.fmt.pix.sizeimage);
				if(!ptr)
				{
					goto fail;
				}
				_videoframes.push_back(std::make_pair(ptr, format.fmt.pix.sizeimage));
			}

			_video_width = format.fmt.pix.width;
			_video_height = format.fmt.pix.height;
			_byteper_line = format.fmt.pix.bytesperline;
			_byteper_frame = format.fmt.pix.sizeimage;
			_video_buffer_count = _videoframes.size();
			return fd;
		}while(0);


fail:
		if(fd > 0)
		{
			close(fd);
			fd = -1;
		}
		return fd;
	}
	int xioctl(int fd, int request, void *arg)
	{
		int res = -1;
		do {
				res = ioctl(fd, request, arg);
		}while(-1 == res && EINTR == errno);
		return res;
	}
	unsigned fourcc(char a, char b, char c, char d)
	{
		return ((unsigned)(a) |
				((unsigned)(b) << 8) |
				((unsigned)(c) << 16) |
				((unsigned)(d) << 24));
	}


	int startpoll(int timeout)
	{
		/*
		 	 wait signal until dirver v4l2 return
		 */






		struct pollfd fds[2];
		memset(fds, 0, sizeof(struct pollfd) * 2);
		fds[0].fd = _fd;
		fds[1].fd = _pipe[0];
		fds[0].events = POLLIN | POLLPRI | POLLRDNORM| POLLERR | POLLHUP | POLLNVAL;
		fds[1].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;


		int res = poll(fds, 2, timeout);

		if(res > 0)
		{

			if(fds[1].revents & (POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL))
			{

				res = -1;
			}
			else if(fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			{

				res = -1;
			}
			else if(fds[0].revents & (POLLIN | POLLPRI | POLLRDNORM))
			{
				res = 1;
			}
		}

		return res;
	}
	bool streamon()
	{
		struct v4l2_buffer buf;
		if(_ing)
		{
			return true;
		}
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		for(int i = 0; i < _video_buffer_count; i++)
		{
			memset(&buf, 0, sizeof(struct v4l2_buffer));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			if(xioctl(_fd, VIDIOC_QBUF, &buf))
			{
				return false;
			}
		}

		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(xioctl(_fd, VIDIOC_STREAMON, &type))
		{
			return false;
		}
		_ing = true;
		return true;
	}
	void streamoff()
	{
	
		if(_ing)
		{
			enum v4l2_buf_type type;
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			ioctl(_fd, VIDIOC_STREAMOFF, &type);
		}

		_ing = false;
		
	}
	void wakeup_pipe()
	{
		if(_pipe[1] > 0)
		{
			char a = 1;
			write(_pipe[1], &a, 1);
		}
	}
public:
	uvc(char const *dev) :
		_dev(dev),
		_support_fourcc(fourcc('Y', 'U', 'Y', 'V')),/*current support*/
		_video_capture_mode(-1),
		_video_width(-1),
		_video_height(-1),
		_videoformat(AV_PIX_FMT_YUYV422),/*current support*/
		_byteper_line(-1),
		_byteper_frame(-1),
		_video_buffer_count(-1),
		_fd(-1),
		_ing(false)
		{
			_fd = uvc_open_test();
			_pipe[0] = _pipe[1] = -1;
			DECLARE_THROW(_fd < 0, "can't open device");
		}
	virtual ~uvc()
	{
		end();
		if(_pipe[0] > 0)close(_pipe[0]);
		if(_pipe[1] > 0)close(_pipe[1]);
		_pipe[0] = _pipe[1] = -1;
	}
	void end()
	{
		stop();
		wakeup_pipe();


		for(auto &it : _videoframes)
		{
			if(_video_capture_mode == 1)
			{
				__base__free__(it.first);
			}
			if(_video_capture_mode == 0)
			{
				munmap(it.first, it.second);
			}
		}

		
		if(_fd > 0)
		{
			close(_fd);
			_fd = -1;
		}
	}
	bool start()
	{
		return streamon();
	}
	void stop()
	{
		return streamoff();
	}
	int waitframe(int timeout)
	{
		int res = -1;
		if(start())
		{

			res = startpoll(timeout);

		}
		return res;
	}


	template <typename functor>
	int get_videoframe(functor &&f)
	{
		
		if(!_ing)
		{
			return -1;
		}

		int res = -1;

		if(_video_capture_mode == 0)
		{

			struct v4l2_buffer buf;
			memset(&buf, 0, sizeof(struct v4l2_buffer));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			if(xioctl(_fd, VIDIOC_DQBUF, &buf))
			{
				return -1;
			}

			if(buf.index > _video_buffer_count)
			{
				return -1;
			}
			{
				pixelframe pix(_video_width,
				_video_height, 
				_videoformat, 
				_videoframes.at(buf.index).first);
				f(pix);
			}


			if(xioctl(_fd, VIDIOC_QBUF, &buf))
			{
				return -1;
			}

			return _videoframes.at(buf.index).second;
		}

		if(_video_capture_mode == 1)
		{

			_dword readbyte = 0;
			size_t remainframe = 0;
			char *pbuff = NULL;

			remainframe = _videoframes.at(0).second;
			pbuff = (char *)_videoframes.at(0).first;
			do
			{
				readbyte = read(_fd, (pbuff + (_videoframes.at(0).second - remainframe)), remainframe);
				if(readbyte < 0)
				{
					switch(errno)
					{
					case EIO:
					case EAGAIN:
						continue;
					default:
						return -1;
					}
				}
				remainframe -= readbyte;
			}while(remainframe > 0);
			


			{
				pixelframe pix(_video_width,
				_video_height, 
				_videoformat, 
				pbuff);
				f(pix);
			}

			return _videoframes.at(0).second;
		}

		return res;
	}
	int video_width() const
	{
		return _video_width;
	}
	int video_height() const
	{
		return _video_height;
	}
	enum AVPixelFormat video_format() const
	{
		return _videoformat;
	}
	int video_fps() const
	{
		return 25;/*dump*/
	}

private:
	char const *_dev;
	unsigned _support_fourcc;
	int _video_capture_mode;
	int _video_width;
	int _video_height;
	enum AVPixelFormat _videoformat;
	int _byteper_line;
	int _byteper_frame;
	int _video_buffer_count;
	int _fd;
	bool _ing;
	int _pipe[2];
	std::vector<std::pair<void *, unsigned>> _videoframes;
};
#endif
