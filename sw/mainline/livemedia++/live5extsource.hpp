#pragma once


/*
	input source from other threads data
*/
template <typename T>/*
							template T 
							only  live5scheduler<T> use for 'register trigger'
						*/

class live5extsource : public FramedSource
{
private:
	live5scheduler<T> &_scheduler;
	/*
		trigger event id from another thread
	*/
	unsigned _trigger_id;
	unsigned _buffersize;
	unsigned char *_buffer;
	unsigned _bufferindx;
	bool _putis_lock;
	/*
		locking method for put 
		lock the put thread if data was full 
	*/
	semaphore _lock_put;
	/*
		input : your thread 
		output : live555 shceduled thread
	*/
	std::mutex _lock;

	virtual void doGetNextFrame()
	{
		/*	stop.
			we offered next frame by event trigger 'dogetnextframe_from_bufer'
		*/
	}
	virtual void doStopgGttingFrames()
	{
		FramedSource::doStopGettingFrames();
	}
	void dogetnextframe_from_buffer()
	{
		return get();
	}
	void get()
	{
		_lock.lock();

		/*
			calculate read avail
		*/
		unsigned read_avail_size = _bufferindx;
		unsigned read_reqeust_size = fMaxSize;
		unsigned remain_size = 0;
		fFrameSize = read_avail_size >= read_reqeust_size ? read_reqeust_size : read_avail_size;
		fNumTruncatedBytes = read_reqeust_size >= fFrameSize ? 0 : read_reqeust_size - fFrameSize;
		remain_size = read_avail_size - fFrameSize;

		/*
			copy data to source or sink.
		*/
		memcpy(fTo, _buffer, fFrameSize);
		if(remain_size)
			memcpy(_buffer, _buffer + fFrameSize, remain_size);
		
		_bufferindx = remain_size;
		/*
			wake up 'put' thread if sleep
		*/
		if(_putis_lock)
		{
			SEMA_unlock(&_lock_put);
		}
		_lock.unlock();


		/*
			pts write
		*/
		gettimeofday(&fPresentationTime, nullptr);

		/*
			deliver to aftergetting function ptr
		*/
		FramedSource::afterGetting(this);		
	}
	public:
		virtual ~live5extsource()
		{
			_lock.lock();
			/*
				wake up 'put' thread if sleep
			*/
			if(_putis_lock)
			{
				SEMA_unlock(&_lock_put);
			}
			_lock.unlock();
			__base__free__(_buffer);
		}
		live5extsource(live5scheduler<T> &scheduler, 
			unsigned trigger_id,
			UsageEnvironment &env,
			unsigned buffersize) :
			FramedSource(env), 
				_scheduler(scheduler),
				_trigger_id(trigger_id),
				_buffersize(buffersize),
				_buffer((unsigned char *)__base__malloc__(_buffersize)),
				_bufferindx(0),
				_putis_lock(false)
			{
				throw_if ti;				
				ti(_buffersize < 0, "live5extsource can't create buffersize zero");
				ti(!_buffer, "live5extsource can't create buffer is null pointer");
				ti(SEMA_open(&_lock_put, 0,1) < 0, "live5extsource can't create semaphore has error");

				_scheduler.register_trigger(_trigger_id, 
					[]()->void{
						/*
							deliver data to request sink or source
						*/
						dogetnextframe_from_buffer();
					}
				);
			}

		void put(unsigned char *ptr, unsigned nptr)
		{
			/*
				invalid parameter
			*/
			unsigned usrindx = 0;
			while(nptr > 0 && 
				ptr)
			{
				_lock.lock();				
				unsigned remain_size = _buffersize - _bufferindx;
				_putis_lock = remain_size <= 0; 				
				if(!_putis_lock)
				{	
					/*
						copy the buffer if available writing size
					*/
					unsigned write_size = remain_size >= nptr ? nptr : remain_size; 				
					memcpy(_buffer + _bufferindx, ptr + usrindx, write_size);
					usrindx += write_size;
					nptr -= write_size;
					_bufferindx += write_size;
				}
				_lock.unlock();
				/*
					thread sleep untill write enable
				*/
				if(_putis_lock)
				{
					SEMA_lock(&_lock_put, INFINITE);
				}
			}	

			/*
				if success writing in the buffer,
				signal sending to scheudler for calling 'get()'
			*/
			if(usrindx)
				_scheduler.trigger(_trigger_id);
		}
};

//=====================================================================

template <typename T>
class live5extsource_uvc :
	public live5extsource<T>
{
private:
	char const *_device;
#if defined have_uvc
	uvc *_uvc;
#endif
	std::thread *_thread_uvc;
	encoder *_enc;
	bool _bloop;
	std::mutex _stream_lock;
	avattr _attr;
	void operator()(avframe_class &frm, avpacket_class &pkt,void * )
	{
	/*
		put data to scheduler
	*/
		live5extsource<T>::put(pkt.raw()->data, pkt.raw()->size);
	}

public:
	live5extsource_uvc(char const *device,
		live5scheduler<T> &scheduler, 
		unsigned trigger_id,
		UsageEnvironment &env) : 
		live5extsource<T>(scheduler, trigger_id, env, 1000),
			_device(device),
#if defined have_uvc
			_uvc(nullptr),
#endif
			_thread_uvc(nullptr),
			_enc(nullptr),
			_bloop(true)
	{
	/*
		
	*/
#if defined have_uvc
			_uvc = new uvc(_device);
			_attr.set(avattr_key::frame_video, avattr_key::frame_video, 0, 0.0);
			_attr.set(avattr_key::width, avattr_key::width, _uvc->video_width(), 0.0);
			_attr.set(avattr_key::height, avattr_key::height, _uvc->video_height(), 0.0);
			_attr.set(avattr_key::pixel_format, avattr_key::pixel_format, (int)_uvc->video_format(), 0.0);
			_attr.set(avattr_key::fps, avattr_key::fps, _uvc->video_fps(), 0.0);
			/*
				perhaps fix value 
			*/
			_attr.set(avattr_key::bitrate, avattr_key::bitrate, 400000, 0.0);
			_attr.set(avattr_key::gop, avattr_key::gop, 10, 0.0);
			_attr.set(avattr_key::max_bframe, avattr_key::max_bframe, 1, 0.0);
			_attr.set(avattr_key::encoderid, avattr_key::encoderid, (int)AV_CODEC_ID_H264, 0.0);
			
			_enc = new encoder(_attr);
			_stream_lock.lock();
			_thread_uvc = new std::thread([&]()->void{
					while(_bloop)
					{
						autolock a(_stream_lock);
						int res = _uvc->waitframe(5000);
						if(res > 0)
						{
							avframe_class frame;
							if(_uvc->get(frame) > 0)
							{
								_enc->encoding(frame,(*this)());
							}
						}
					}
				});	
#endif
	}
	virtual ~live5extsource_uvc()
	{
		_stream_lock.unlock();
		_bloop = false;
		if(_thread_uvc)
		{
			_thread_uvc->join();
			delete  _thread_uvc;
		}
#if defined have_uvc
		if(_uvc) delete _uvc;
#endif
		if(_enc) delete _enc;
	}
	virtual void doGetNextFrame()
	{
		/*	
			we use this function that start frame flag
		*/
		_stream_lock.unlock();
	}
	virtual void doStopgGttingFrames()
	{
		live5extsource<T>::doStopGettingFrames();
		_stream_lock.try_lock();
	}
	bool has_video()
	{
		return !_attr.notfound(avattr_key::frame_video);
	}
	int video_width()
	{
		return _attr.get_int(avattr_key::width,-1);
	}
	int video_height()
	{
		return _attr.get_int(avattr_key::height,-1);
	}	
	enum AVPixelFormat videoformat()
	{
		return (enum AVPixelFormat)_attr.get_int(avattr_key::pixel_format,-1);
	}
	int video_fps()
	{
		return _attr.get_int(avattr_key::fps,-1);
	}
	int video_bitrate()
	{
		return _attr.get_int(avattr_key::bitrate,-1);
	}
	int video_gop()
	{
		return _attr.get_int(avattr_key::gop,-1);
	}
	int video_max_bframe()
	{
		return _attr.get_int(avattr_key::max_bframe,-1);
	}
	enum AVCodecID video_codec()
	{
		return (enum AVCodecID)_attr.get_int(avattr_key::encoderid,-1);
	}
};

