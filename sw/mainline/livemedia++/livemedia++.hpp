#pragma once




class livemedia_pp
{
/*
	declare inner class's
*/
public:

	/*
		 throw!
	 */
	struct throw_if
	{
	private: void trace_thread()
		{
			char const *str = nullptr;
			threadid throw_at = BackGround_id();
			livemedia_pp::ref()->namedthread_get(throw_at,str);			
			printf("throw except thread at [%s]\n", str ? str : "unknown");
		}
	private: void trace_address(const char *str, unsigned backtrace_dep)
		{
			void *backtrace_addr [backtrace_dep] = { nullptr, };
			int size = 0;
			if(backtrace_dep)
			{
				size = backtrace(backtrace_addr, backtrace_dep);
			}
	
			if(size > 0)
			{
				printf("terminate called (%s) : look up addr \n", str);
	
				for(int i = 0; i < size; i++)
				{
					printf("addr2line -f -C -e avfor %08x\n", (unsigned)backtrace_addr[i]);
				}
			}
			else
			{
				printf("terminate called (%s): look up unknown	addr\n", str);
			}
		}
	private: void start_exit()
		{
			exit(0);
		}
		
	 public: void operator ( )	( bool likely,
				const char *str = "runtime error",
				unsigned  backtrace_dep = 30)
		{
			if(likely)
			{
				printf("==================================\n");
				printf("         EXCEPTION CALL \n");
				printf("==================================\n");
				trace_thread();
				trace_address(str, backtrace_dep);

				start_exit();
			}
		}
	};

	


	class namedthread final
	{
		friend class livemedia_pp;
		/*
			pairing string to threadid
		*/
		char const *_name ;
		threadid _tid;
	public:
		namedthread(char const *name = "unknown") : 
			_name(name),
			_tid(BackGround_id())
		{
			throw_if()
			(!livemedia_pp::ref()->namedthread_register(this), 
			"can't register named thread");
		}
		namedthread()
		{
			livemedia_pp::ref()->namedthread_unregister(this);
		}
		char const *name()
		{
			return _name;
		}
		threadid id()
		{
			return _tid;
		}
		bool operator()()
		{
			return _name != nullptr;			
		}
		bool operator== (  threadid id)
		{
			return this->id() == id;
		}
		bool operator == (char const *name)
		{
			if(name)
			{
				return !strcmp(name, this->name());
			}
		}
		bool operator== (  namedthread &t)
		{
			if((*this)()&&
				t() &&
				(t._tid == _tid &&
				!strcmp(t._name, _name)))
			{
				return true;
			}
		
			return false;
		}
	};
private:
	struct throw_register_sys_except
	{
		static void sys_except(int nsig)
		{
			throw_if()(1, nsig == SIGSEGV ? "sigsegv" : "sigabrt");
		}
		throw_register_sys_except()
		{
			signal(SIGSEGV, throw_register_sys_except::sys_except);
			signal(SIGABRT, throw_register_sys_except::sys_except);
		}
	};
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
	
	
				throw_if()(!_ptr, "can't create swcontext");
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
			{
				std::mutex *t = (std::mutex *)*arg;
				delete t;
			}
	
			return 0;
		}

public:
	livemedia_pp()
	{
		avcodec_register_all();
		av_register_all();
		av_lockmgr_register(ffmpeg_lockmgr) ;
	//	throw_register_sys_except();
	}
	virtual ~livemedia_pp()
	{
		_filercontext_container.clear();
		_namedthreads.clear();
	}

	/*
	 	 create or reference the instance
	 */
        static livemedia_pp*ref(bool bc = true)
        {
            static livemedia_pp *obj = nullptr;
            if(bc)
            {
                if(!obj)
                {
                    libwq_heap_testinit();
                    obj = new livemedia_pp;
                }
            }
            else
            {
                if(obj)
                {
                    delete obj;
                    libwq_heap_testdeinit();
                }
                obj = nullptr;
            }
            return obj;
        }

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

	
	bool namedthread_register( namedthread *t )
	{
	
		if(t && 
			(*t)())		
		{
			autolock a(_namedthreads_mutex);
			for(auto it : _namedthreads)
			{
				if((*it) == (*t))
				{
					/*
						duplicated register info
					*/			
					return false;
				}
			}
				printf("registed named thread : %s\n", t->_name);
			_namedthreads.push_back(t);
			return true;
		}
		
		return false;
	}
	void namedthread_unregister(namedthread *t )
	{

		if(t)
		{
			autolock a(_namedthreads_mutex);
			_namedthreads.remove_if([&](namedthread *a)->bool{
					(*a) == (*t);
				});
		}		
	}
	bool namedthread_get(threadid id, char const *&out)
	{
		autolock a(_namedthreads_mutex);
		for(auto it : _namedthreads)
		{
			if((*it) == id)
			{
				out = it->_name;
				return true;
			}
		}
		return false;
	}
	bool namedthread_get(char const *n, threadid &out)
	{
		autolock a(_namedthreads_mutex);
		for(auto it : _namedthreads)
		{
			if((*it) == n)
			{
				out = it->_tid;
				return true;
			}
		}
		return false;
	}

private:


	std::mutex _filtercontext_container_mutex;
	std::mutex _namedthreads_mutex;
	std::vector<filtercontext_container>_filercontext_container;
	std::list<namedthread *>_namedthreads;
};

typedef livemedia_pp::namedthread livemedia_namedthread;
typedef livemedia_pp::throw_if	livemedia_throw_if;
/*
	var 'nt' always single instance in one thread
*/
#define DECLARE_LIVEMEDIA_NAMEDTHREAD(a)	livemedia_namedthread nt(a);
#define DECLARE_THROW(cond, c)	  livemedia_throw_if()(cond, c);
