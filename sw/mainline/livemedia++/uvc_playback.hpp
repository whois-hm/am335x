#pragma once
//
//	struct uvcreader
//	{
//
//		/*
//		 	 controled outside thread 'uvc_playback',
//		 	 reading frame this reader thread operator()
//		 */
//		uvc *_uvc;
//		avattr *_attr_ref;
//		avframebuffering *_scheduler_ref;
//		std::mutex *_scheduler_lock_ref;
//		bool _bstart;
//		uvcreader(char const *name,
//				avattr *attr,
//				avframebuffering *_scheduler,
//				std::mutex *_mutex) :
//			_uvc(new uvc(name)),
//			_attr_ref(attr),
//			_scheduler_ref(_scheduler),
//			_scheduler_lock_ref(_mutex),
//			_bstart(false)
//		{throw_if ().operator ()(!_uvc);}
//		~uvcreader(){
//				delete _uvc;
//		}
//		void operator()(uvc_playback *thiz)
//		{
//			while(_uvc->waitframe(-1) > 0)
//			{
//				pixelframe pixframe;
//				_uvc->get(pixframe);
//
//				/*
//				 	 uvc fps nomarlly upto 30
//				 	 so, convert frame in this thread.
//				 	 time is enuough
//				 */
//				swxcontext_class ((pixframe), (*_attr_ref));
//				autolock a(*_scheduler_lock_ref);
//				(*_scheduler_ref) << pixframe;
//			}
//		}
//		void end()
//		{
//			_uvc->end();
//		}
//		void stop()
//		{
//			if(_bstart)
//			{
//				_uvc->stop();
//				_bstart = false;
//			}
//		}
//		void start()
//		{
//			if(!_bstart)
//			{
//				_uvc->start();
//				_bstart = true;
//			}
//		}
//		enum AVMediaType getmasterclock()
//		{
//
//		}
//
//	};
//
class uvc_playback :
		public playback_inst
{

private:

	uvc *_uvc;

public:
	uvc_playback(const avattr &attr, char const *name) :
		playback_inst(attr),
		_uvc(nullptr)
	{
		throw_if ()(attr.notfound(avattr_key::frame_video));
		_uvc = new uvc(name);
	}
	virtual ~uvc_playback()
	{
		delete _uvc;
	}
	enum AVMediaType get_master_clock()
	{
		return AVMEDIA_TYPE_VIDEO;
	}
	void pause()
	{
		_uvc->stop();
	}
	void resume(bool closing = false)
	{
		_uvc->start();
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

		/*
		 	 we do not have to pts scheduling.
		 	 just schedule dependent uvc driver's fps
		 */
		int res = _uvc->waitframe(500);
		if(res > 0)
		{
			avframe_class frame;
			if(_uvc->get(frame) > 0)
			{
				pixelframe pixf(*frame.raw());

				swxcontext_class ((pixf), (_attr));
				pixf >> output;
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



