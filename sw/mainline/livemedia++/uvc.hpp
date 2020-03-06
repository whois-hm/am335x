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
				printf("pipe open fail\n");
				goto fail;
			}
			desc.index = indx;
			desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			req.count = 4;
			req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			req.memory = V4L2_MEMORY_MMAP;


			fd = open(_dev, O_RDWR | O_NONBLOCK , 777);
			if(fd < 0)
			{
				printf("device : %s  open fail\n", _dev);
				goto fail;
			}
			if(xioctl(fd, VIDIOC_QUERYCAP, &cap))
			{
				printf("can't get vidioc query\n");
				goto fail;
			}
			if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
			{
				printf("can't find video capture\n");
				goto fail;
			}
			if(cap.capabilities & V4L2_CAP_STREAMING)	 _video_capture_mode = 0;
			else if(cap.capabilities & V4L2_CAP_READWRITE)	 _video_capture_mode = 1;
			else
			{
				printf("can't find video capture\n");
				goto fail;
			}
			if(xioctl(fd, VIDIOC_G_FMT, &format))
			{
				printf("can't get format\n");
				goto fail;
			}
			if(format.fmt.pix.pixelformat != _support_fourcc)
			{
				printf("no support video format...\n");
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
				printf("no support video format...\n");
				goto fail;
			}


			/*
			 	 mode 0 mean that io mmap mode
			 */
			if(_video_capture_mode == 0)
			{
				if((xioctl(fd, VIDIOC_REQBUFS, &req)))
				{
					printf("can't request qbuffers\n");
					goto fail;
				}
				if(req.count < 2)
				{
					printf("request qbuffer has too short\n");
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
						printf("can't get query buffer\n");
						goto fail;
					}
					void *ptr = mmap(NULL, buf.length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, buf.m.offset);
					if(MAP_FAILED == ptr)
					{
						printf("mmap frame fail\n");
						goto fail;
					}
					_videoframes.push_back(std::make_pair(ptr, buf.length));
				}
				for(int i = 0; i < req.count; i++)
				{
					memset(&buf, 0, sizeof(struct v4l2_buffer));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index = i;
					if(xioctl(fd, VIDIOC_QBUF, &buf))
					{
						printf("can't qbuf\n");
						goto fail;
					}
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
					printf("frame malloc fail\n");
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

	int read_io(void *&f)
	{
		f = nullptr;
		if(_video_capture_mode != 0)
		{
			return 0;
		}
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if(xioctl(_fd, VIDIOC_DQBUF, &buf))
		{
			return -1;
		}
		if(xioctl(_fd, VIDIOC_QBUF, &buf))
		{
			return -1;
		}
		if(buf.index > _video_buffer_count)
		{
			return -1;
		}

		f = _videoframes.at(buf.index).first;
		return _videoframes.at(buf.index).second;
	}
	int read_rw(void *&f)
	{
		f = nullptr;
		if(_video_capture_mode != 1)
		{
			return 0;
		}
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
		f = pbuff;
		return _videoframes.at(0).second;
	}
	int startpoll(int timeout)
	{
		/*
		 	 wait signal until dirver v4l2 return
		 */
		struct pollfd fds[2];
		memset(fds, 0, sizeof(struct pollfd) * 2);
		fds[0].fd = _fd;
		fds[1].fd = _pipe[1];
		fds[0].events = fds[1].events = POLL_IN | POLL_PRI | POLL_ERR;

		int res = poll(fds, 2, timeout);
		if(res > 0)
		{
			if(fds[1].revents & (POLL_IN | POLL_PRI | POLL_ERR))
			{
				res = -1;
			}
			if(fds[0].revents & POLL_ERR)
			{
				res = -1;
			}
			if(fds[0].revents & (POLL_IN | POLL_PRI))
			{
				res = 1;
			}
		}
		return res;
	}
	bool streamon()
	{
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(xioctl(_fd, VIDIOC_STREAMON, &type))
		{
			return false;
		}
		return true;
	}
	void streamoff()
	{
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl(_fd, VIDIOC_STREAMOFF, &type);
	}
	void wakeup_pipe()
	{
		if(_pipe[0] > 0)
		{
			char a = 1;
			write(_pipe[0], &a, 1);
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
		_fd(uvc_open_test())
		{
			_pipe[0] = _pipe[1] = -1;
			throw_if ti; ti(_fd < 0, "can't open device");
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
		return startpoll(timeout);
	}

	int get(avframe_class &f)
	{
		f.unref();
		int res = -1;
		void *ptr = nullptr;

		res = read_io(ptr);
		if(res == 0)
		{
			res = read_rw(ptr);
		}
		if(res > 0)
		{
			/*
			 	 frame obtain get driver and put the 'f'
			 	 with base value, other pts ... etc is not fill

			 */
			f.raw()->width = _video_width;
			f.raw()->height = _video_height;
			f.raw()->format = _videoformat;
			av_frame_get_buffer(f.raw(), 0);

			av_image_fill_arrays(f.raw()->data,
					f.raw()->linesize,
					(uint8_t *)ptr,
					_videoformat,
					_video_width,
					_video_height,
					1);
			return res;
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
	int _pipe[2];
	std::vector<std::pair<void *, unsigned>> _videoframes;
};
#endif
