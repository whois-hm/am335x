#pragma once
#if defined have_uvc
class uvc_playback :
		public playback_inst
{


	struct recording
	{
		friend class uvc_playback;
		enum event { open, record, close, exit, };
		struct par
		{
			enum event e;
			union
			{
				struct {char const *file;
				int width;
				int height;
				enum AVPixelFormat fmt;
				int fps;
				int bitrate;
				int gop;
				int bframe;
				enum AVCodecID cid;}open;
				struct { pixelframe *pixel; }record;
				struct { }close;
				struct { }exit;
			};
		};
		mediacontainer_record *_rec;
		avattr _attr;
		recording() : _rec(nullptr){}
		~recording(){fn_close(nullptr);}
		void fn_open(recording::par *p)
		{
			fn_close(nullptr);
			avattr attr;
			attr.set(avattr_key::frame_video, avattr_key::frame_video, 0, 0.0);
			attr.set(avattr_key::width, avattr_key::width, p->open.width, 0.0);
			attr.set(avattr_key::height, avattr_key::height, p->open.height, 0.0);
			attr.set(avattr_key::pixel_format, avattr_key::pixel_format, (int)p->open.fmt, 0.0);
			attr.set(avattr_key::fps, avattr_key::fps, p->open.fps, 0.0);
			attr.set(avattr_key::bitrate, avattr_key::bitrate, p->open.bitrate, 0.0);
			attr.set(avattr_key::gop, avattr_key::gop, p->open.gop, 0.0);
			attr.set(avattr_key::max_bframe, avattr_key::max_bframe, p->open.bframe, 0.0);
			attr.set(avattr_key::video_encoderid, avattr_key::video_encoderid, (int)p->open.cid, 0.0);
			_attr = attr;
			_rec = new mediacontainer_record(_attr,
					avattr(),/*no audio source*/
					p->open.file);
		}
		void fn_record(recording::par *p)
		{
			if(_rec)
			{
				{
					swxcontext_class (*p->record.pixel, _attr);
				}

				_rec->recording(*p->record.pixel);
			}

			delete p->record.pixel;
		}
		void fn_close(recording::par *p)
		{
			 if(_rec)delete _rec;
			 _rec = nullptr;
		}

		bool operator()(_dword r, void *par, _dword size, _dword *qtime)
		{
			if(r != WQOK)
			{
				return false;
			}
			recording::par *args = (recording::par *)par;
			if(args->e == open) { fn_open(args);return true;}
			else if(args->e == record) { fn_record(args); return true; }
			else if(args->e == close) { fn_close(args); return true; }
			else if(args->e == exit) { fn_close(nullptr); return false; }
			return false;
		}
	};
	class pixelframe_filter :
			public filter<pixelframe,
			uvc_playback>
	{
		friend class uvc_playback;
		pixelframe_filter(uvc_playback *p) : filter<pixelframe,
				uvc_playback>(p){}
		virtual void operator >> (pixelframe &pixel)
		{
			recording::par par;
			par.e = recording::record;
			par.record.pixel = new pixelframe(pixel);
			ptr()->_recthread.sendto(&par, sizeof(par), INFINITE);
		}
	};
private:

	uvc *_uvc;
	wthread<recording> _recthread;
	pixelframe_filter _pixelfilter;
	std::mutex _lock;



public:
	uvc_playback(const avattr &attr, char const *name) :
		playback_inst(attr),
		_uvc(nullptr),
		_recthread(wthread<recording>(10, sizeof(recording::par), "uvc rec thread")),
		_pixelfilter(pixelframe_filter(this))
	{
		DECLARE_THROW(attr.notfound(avattr_key::frame_video), "uvc playback can't found video frame");
		_recthread.start(INFINITE);
		_uvc = new uvc(name);
		pause();
	}
	virtual ~uvc_playback()
	{
		recording::par par;
		par.e = recording::exit;
		_recthread.sendto(&par, sizeof(par), INFINITE);
		delete _uvc;
	}
	enum AVMediaType get_master_clock()
	{
		return AVMEDIA_TYPE_VIDEO;
	}
	void record(char const *file)
	{
		/*
		 	 no condition pause / resume
		 */
		{
			recording::par par;
			par.e = recording::close;
			_recthread.sendto_wait(&par, sizeof(par), INFINITE);
			_pixelfilter.disable();
		}
		if(!file)
		{
			/*
			 	 null pointer mean that record stop!
			 */
			return;
		}
		recording::par par;
		par.e = recording::open;
		par.open.bframe = 1;
		par.open.bitrate = 400000;
		par.open.cid = AV_CODEC_ID_H264;
		par.open.file = file;
		/*
		 	 hack : force yuyv420p for h264
		 */
		//par.open.fmt = _uvc->video_format();
		par.open.fmt = AV_PIX_FMT_YUV420P;
		par.open.fps = _uvc->video_fps();
		par.open.gop = 10;
		par.open.height = _uvc->video_height();
		par.open.width = _uvc->video_width();
		_recthread.sendto_wait(&par, sizeof(par), INFINITE);
		_pixelfilter.enable();
	}
	void pause()
	{
		if(_lock.try_lock())
		{
			_uvc->stop();
		}
	}
	void resume(bool closing = false)
	{
		_lock.unlock();
	}
	void seek(double incr)
	{
		/*can't seek camera*/
	}

	bool has(avattr::avattr_type_string &&key)
	{
		if(key == avattr_key::frame_video)
		{
			return true;
		}
		return false;
	}
	void play()
	{
		resume();
	}
    duration_div duration()
    {
    	/*can't know duration */
    	return duration_div (-1,-1,-1,-1,-1);
    }
	virtual int take(const std::string &title, pixel &output)
	{

		autolock a(_lock);
		/*
		 	 we do not have to pts scheduling.
		 	 just schedule dependent uvc driver's fps
		 */
		int res = _uvc->waitframe(5000);
		if(res > 0)
		{

			if(_uvc->get_videoframe([&](pixelframe &pix)->void{
					swxcontext_class (pix, (_attr));
					_pixelfilter << pix;
					pix >> output;
				}) <= 0)
			{
				res = -1;
			}

			if(!output.can_take())
			{

				/*
				 	 internal error
				 */
				res = -1;
			}

		}
		return res;
	}
	virtual int take(const std::string &title, pcm_require &output)
	{
		return -1;
	}
};


#endif
