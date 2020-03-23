#pragma once


/*
	input source from other threads data
*/


class live5extsource : public FramedSource
{

protected:
	void buffer_clear()
	{
		_lock.lock();
		_bufferindx = 0;
		if(_putis_lock)
		{
			SEMA_unlock(&_lock_put);
			_putis_lock = false;
		}
		_lock.unlock();
	}
private:
	/*
	
		trigger event id from another thread
		don't use live5scheduler<type>'s api, because avoid dependency
	*/
	EventTriggerId _id;


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


	static void dogetnextframe_frm_buffer1(void *c)
	{
		live5extsource * s = (live5extsource *)c;
		s->dogetnextframe_from_buffer();
	}
	void dogetnextframe_from_buffer()
	{
		return get();
	}

	void get()
	{
		if(!isCurrentlyAwaitingData())
		{

			return;
		}

		_lock.lock();
		/*
			calculate read avail
		*/
		unsigned read_avail_size = _bufferindx;
		unsigned read_reqeust_size = fMaxSize;
		unsigned remain_size = 0;
		fFrameSize = read_avail_size >= read_reqeust_size ? read_reqeust_size : read_avail_size;
		fNumTruncatedBytes = read_reqeust_size >= fFrameSize ? read_reqeust_size - fFrameSize : 0;
		remain_size = read_avail_size - fFrameSize;
		if(!fFrameSize)
		{
			_lock.unlock();
			return;
		}

		/*
			copy data to source or sink.
		*/
		memcpy(fTo, _buffer, fFrameSize);

		if(remain_size)
			memcpy(_buffer, _buffer + fFrameSize, remain_size);


		_bufferindx = remain_size;
		//printf("framesize = %d request size = %d remain size = %d truncatebyte = %d\n", fFrameSize,read_reqeust_size, remain_size, fNumTruncatedBytes);
		/*
			wake up 'put' thread if sleep
		*/
		if(_putis_lock)
		{
			SEMA_unlock(&_lock_put);
			_putis_lock = false;
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
protected:
	virtual void doGetNextFrame()
	{
		/*	stop.
			we offered next frame by event trigger 'dogetnextframe_from_bufer'
			just save 'FramedSource::afterGetting(this);'s parameter, or start flag
		*/
		get();
	}

	virtual void doStopgGttingFrames()
	{
		FramedSource::doStopGettingFrames();
	}
	public:
		virtual ~live5extsource()
		{
			/*
				wake up 'put' thread if sleep
			*/
			buffer_clear();

			__base__free__(_buffer);
			envir().taskScheduler().deleteEventTrigger(_id);
		}
		live5extsource(
			UsageEnvironment &env,
			unsigned buffersize) :
			FramedSource(env), 
				_id(0),
				_buffersize(buffersize),
				_buffer((unsigned char *)__base__malloc__(_buffersize)),
				_bufferindx(0),
				_putis_lock(false)
			{			
				DECLARE_THROW(_buffersize < 0, "live5extsource can't create buffersize zero");
				DECLARE_THROW(!_buffer, "live5extsource can't create buffer is null pointer");
				DECLARE_THROW(SEMA_open(&_lock_put, 0,1) < 0, "live5extsource can't create semaphore has error");

				_id = env.taskScheduler().createEventTrigger(dogetnextframe_frm_buffer1);
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
				envir().taskScheduler().triggerEvent(_id, this);
		}
};

//=====================================================================
//class uvc source 
//


class live5extsource_uvc :
	public live5extsource
{
private:
	char const *_device;
#if defined have_uvc
	uvc *_uvc;
#endif
	std::thread *_thread_uvc;
	encoder *_enc;
	bool _bloop;
	/*
	 	 this flag controled by one thread 'live555 scheduler.
	 	 this flag use for 'stream_lock' one controled
	 */
	bool _bgetnextframe;
	std::mutex _stream_lock;
	avattr _attr;


public:
	live5extsource_uvc(char const *device,
		UsageEnvironment &env) : 
		live5extsource(env, 10000),
			_device(device),
#if defined have_uvc
			_uvc(nullptr),
#endif
			_thread_uvc(nullptr),
			_enc(nullptr),
			_bloop(true),
			_bgetnextframe(false)
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
			_attr.set(avattr_key::video_encoderid, avattr_key::video_encoderid, (int)AV_CODEC_ID_H264, 0.0);

			{
				/*
					......  test code ,  delete later
					lib264 can't support if camera's pixel format
					lib264 has encoding format yuv420p only, current build option.....
				*/
				encoder encodertest;
				if(AV_PIX_FMT_YUV420P != _uvc->video_format() &&
					encodertest.parameter_open_test(_attr))
				{
					_attr.reset(avattr_key::pixel_format, avattr_key::pixel_format, (int)AV_PIX_FMT_YUV420P, 0.0);
				}				
			}
			
			_enc = new encoder(_attr);
			/*
			 	 first _bgetnextframe(false)
			 */
			_stream_lock.lock();
			_thread_uvc = new std::thread([&]()->void{
				DECLARE_LIVEMEDIA_NAMEDTHREAD("live5extsource_uvc");
					while(1)
					{						
						autolock a(_stream_lock);
						if(!_bloop)
						{
							break;
						}

						int res = _uvc->waitframe(10000);
						/*
						 	 because enough time for get video,
						 	 return value invalid has mean that uvc error....
						 */
						DECLARE_THROW(res <= 0, "source uvc has broken");


						_uvc->get_videoframe([&](pixelframe &pix)->void{
								{
									swxcontext_class (pix, _attr);
								}
								int res = _enc->encoding(pix,(*this));
							});



					
					}
				});	
#endif
	}
	virtual ~live5extsource_uvc()
	{
		_bloop = false;

		wakeup_threaduvc();
		wakeup_threaduvc_if_puting();

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
	void wakeup_threaduvc()
	{
		/*	
			we use this function that start frame flag
		*/
		if(!_bgetnextframe)
		{
			/*
			 	 wake up thread
			 */
			_stream_lock.unlock();
			_bgetnextframe = true;
		}
	}
	void wakeup_threaduvc_if_puting()
	{
		buffer_clear();
	}
	void sleep_threaduvc()
	{
		if(_bgetnextframe)
		{
			/*
			 	 sleep thread
			 */
			_bgetnextframe = false;
			_stream_lock.lock();
		}
	}
	void operator()(avframe_class &frm, avpacket_class &pkt,void * )
	{
		/*
			put data to scheduler.
			called another thread
		*/
		live5extsource::put(pkt.raw()->data, pkt.raw()->size);
	}
	virtual void doGetNextFrame()
	{
		wakeup_threaduvc();
		/*semantic code. nothing work*/
		live5extsource::doGetNextFrame();
	}
	virtual void doStopgGttingFrames()
	{
		sleep_threaduvc();
		live5extsource::doStopGettingFrames();
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
		return (enum AVCodecID)_attr.get_int(avattr_key::video_encoderid,-1);
	}
};

