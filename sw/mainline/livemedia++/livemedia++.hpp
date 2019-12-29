#pragma once



class livemedia_pp
{
public:
	livemedia_pp()
	{
		avcodec_register_all();
		av_register_all();
		av_lockmgr_register(ffmpeg_lockmgr) ;
	}
	virtual ~livemedia_pp()
	{
		_filercontext_container.clear();
	}

	/*
	 	 create or reference the instance
	 */
	static livemedia_pp*ref();
	/*
	 	 delete our instance
	 */
	static void unref();

	/*
	 	 	 return the matching convert context of ffmpeg

	 	 	 calling this function lock the return context,
	 	 	 then you need calling function 'unlock_filtercontext' for unlock
	 */
	void * lock_filtercontext(const compare_triple_int &val,
			enum AVMediaType type)
	{
		_filtercontext_container_mutex.lock();

		for(auto &it : _filercontext_container)
		{
			if(it(val, type))
			{
				it.lock();
				_filtercontext_container_mutex.unlock();
				return it.getptr();
			}
		}
		filtercontext_container _con(val, type);

		_filercontext_container.push_back(_con);
		_filtercontext_container_mutex.unlock();
		return lock_filtercontext(val, type);
	}
	/*
	 	 	 unlock convert context
	 */
	void unlock_filtercontext(void *&context)
	{
		_filtercontext_container_mutex.lock();
		for(auto &it : _filercontext_container)
		{
			if(it(context))
			{
				context = nullptr;
				it.unlock();
				break;
			}
		}
		_filtercontext_container_mutex.unlock();

	}
private:
	/*
	 	 	 convert context of ffmpeg stored class
	 */
	class filtercontext_container
	{
public:
		compare_triple_int _val;
		void *_ptr;
		enum AVMediaType _t;
		std::mutex _mutex;

		~filtercontext_container()
		{
			if(!_ptr)
			{
				return;
			}
			if(_t == AVMEDIA_TYPE_VIDEO )
			{
				sws_freeContext((struct SwsContext *)_ptr);
			}
			if(_t == AVMEDIA_TYPE_AUDIO)
			{
				swr_free((struct SwrContext **)&_ptr);
			}
			_ptr = nullptr;
		}
		filtercontext_container(const compare_triple_int &val,
				enum AVMediaType t) :
						_val(val),
						_ptr(nullptr),
						_t(t)
		{
			throw_if ti;

			if (t == AVMEDIA_TYPE_VIDEO)
			{
				struct SwsContext *_sws = sws_getContext(std::get<0>(_val.first),
						std::get<1>(_val.first),
						(enum AVPixelFormat)std::get<2>(_val.first),
						std::get<0>(_val.second),
						std::get<1>(_val.second),
						(enum AVPixelFormat)std::get<2>(_val.second),
						SWS_FAST_BILINEAR,
						NULL,
						NULL,
						NULL);


				_ptr = (void *)_sws;




			}
			if( t == AVMEDIA_TYPE_AUDIO )
			{
				struct SwrContext *_swr = swr_alloc_set_opts(NULL,
						av_get_default_channel_layout(std::get<0>(_val.second)),
						(enum AVSampleFormat)std::get<2>(_val.second),
						std::get<1>(_val.second),
						av_get_default_channel_layout(std::get<0>(_val.first)),
						(enum AVSampleFormat)std::get<1>(_val.first),
						std::get<2>(_val.first),
						0,
						NULL);
				_ptr = (void *)_swr;
			}


			ti(!_ptr, "can't create swcontext");
		}
		filtercontext_container( const filtercontext_container &cp) :
				_val(cp._val),
				_ptr(cp._ptr),
				_t(cp._t)
		{
			/*
			 		we reuse context, clear the rvalue
			 */
			filtercontext_container &dump = const_cast<filtercontext_container &>(cp);
			dump._ptr = nullptr;
			dump._t = AVMEDIA_TYPE_UNKNOWN;
		}
		void lock()
		{
			_mutex.lock();
		}
		void unlock()
		{
			_mutex.unlock();
		}
		void *getptr()
		{
			return _ptr;
		}
		enum AVMediaType gettype()
		{
			return _t;
		}

		bool operator () (const compare_triple_int &val,
				enum AVMediaType t) const
		{
			return t == _t&&
					_val == val;
		}
		bool operator () (void *ptr)
		{
			return _ptr && ptr && (_ptr == ptr);
		}
	};
	static int ffmpeg_lockmgr(void **arg, enum AVLockOp op)
	{
		if(op == AV_LOCK_CREATE)
			*arg = new std::mutex();
		else if(op == AV_LOCK_OBTAIN)
			((std::mutex *)*arg)->lock();
		else if(op == AV_LOCK_RELEASE)
			((std::mutex *)*arg)->unlock();
		else if(op == AV_LOCK_DESTROY)
			delete *arg;
		return 0;
	}

	static livemedia_pp *obj;
	std::mutex _filtercontext_container_mutex;
#if memory_trace_level >= 2
	std::mutex _global_mutex;
#endif
	std::vector<filtercontext_container>_filercontext_container;
};


