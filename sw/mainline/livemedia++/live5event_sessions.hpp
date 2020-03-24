
#pragma once

namespace live5
{
class event_servermediasubession;
/*foward declare */
struct stream
{
	/*
	 	 thiz value access live5scheduler only
	 */
	bool _bstream_lock;
	bool _bloop;
	int _pid;
	std::thread *_th;
	std::mutex _stream_lock;
	/*
	 	 managing sussessions list
	 */
	std::list<event_servermediasubession *>_mysubsessions;

	stream(unsigned id) : _bstream_lock(false),
			_bloop(true),
			_pid(id),
			_th(nullptr)
	{sleep();/*first pause stream*/}

	virtual ~stream()
	{
		end();
	}
	void wakeup()
	{
		if(_bstream_lock)
		{
			_stream_lock.unlock();
			_bstream_lock = false;
		}
	}
	void sleep()
	{
		if(!_bstream_lock)
		{
			_stream_lock.lock();
			_bstream_lock = true;
		}
	}
	virtual void start(struct stream *thiz) = 0;
	virtual unsigned registed_subsession() = 0;
	virtual void detach_subsessions() = 0;/*no need virtual but for reference compile error*/

	void start()
	{
		/*
		 	 starting device reading
		 */
		_th = new std::thread([&]()->void{start(this);});
	}
	void end()
	{
			/*detach stream reading thread*/
		if(_th)
		{
			_bloop = false;
			detach_subsessions();

			wakeup();
			_th->join();
			delete _th;
			_th = nullptr;
		}
	}
};
//--------------------------------------------------------------------------------------
// source : do it recv signaled
//--------------------------------------------------------------------------------------
class eventsource : public FramedSource
{
public:
	void buffer_clear()
	{
		/*
		 	 useful wakeup puting thread
		 */
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
	stream *_parent;
	unsigned _operator_id;
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
		eventsource * s = (eventsource *)c;
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
			... but if readable buffer get the data
		*/
		_parent->wakeup();
		get();
	}

	virtual void doStopgGttingFrames()
	{
		_parent->sleep();
		FramedSource::doStopGettingFrames();
	}
	public:
		virtual ~eventsource()
		{
			/*
				wake up 'put' thread if sleep
			*/
			buffer_clear();

			__base__free__(_buffer);
			envir().taskScheduler().deleteEventTrigger(_id);
		}
		eventsource( unsigned operator_id,
			stream *parent,
			UsageEnvironment &env,
			unsigned buffersize) :
			FramedSource(env), 
				_id(0),
				_parent(parent),
				_operator_id(operator_id),
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
		unsigned subsession_pair_id()
		{
			return (_operator_id & 0xffff0000) >> 16;
		}
		enum AVMediaType subsession_pair_mediaid()
		{
			return (enum AVMediaType)(_operator_id & 0x0000ffff);
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


//--------------------------------------------------------------------------------------
// mediasubsessions
//--------------------------------------------------------------------------------------
class event_servermediasubession :
		public OnDemandServerMediaSubsession
{
		/*based subsession*/

protected:
	eventsource *_devsource;
	FramedSource *_source;
	RTPSink *_sink;

	event_servermediasubession(UsageEnvironment &env, unsigned id, stream *parent) :
		OnDemandServerMediaSubsession(env, True/*reuse source*/),
		_devsource(new eventsource(id, parent, env, 10000)), _source(nullptr), _sink(nullptr) {}
	virtual ~event_servermediasubession()
	{
		if(_source)			 { Medium::close(_source); }
		if(_devsource) 		 { Medium::close(_devsource); }
		if(_sink) 			 { Medium::close(_sink); }
	}
	virtual void closeStreamSource(FramedSource *inputSource)
	{ /*do not close*/ /*we pattern are sigletone*/}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate) = 0;
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource) = 0;

public:
	void detach_buffer()
	{
		/*for stream thread joinable*/
		_devsource->buffer_clear();
	}
	/* put data to network*/
	void send(unsigned pid,
			enum AVMediaType mediaid,
			unsigned char *pdata,
			unsigned ndata)
	{
		if(_devsource->subsession_pair_id() == pid &&
				_devsource->subsession_pair_mediaid() == mediaid)
		{
			_devsource->put(pdata, ndata);
		}
	}
};
class event_h264_servermediasubsession : public event_servermediasubession
{
		/*source is h264 out is h264*/
public:
	event_h264_servermediasubsession(UsageEnvironment &env, unsigned id, stream *parent) : event_servermediasubession(env, id, parent){}
	virtual ~event_h264_servermediasubsession(){}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
						      unsigned& estBitrate)
	{
		if(!_source)
		{
			_source = H264VideoStreamFramer::createNew(envir(), _devsource);
		}
		return _source;
	}
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
					unsigned char rtpPayloadTypeIfDynamic,
					FramedSource* inputSource)
	{
		if(!_sink)
		{
			_sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
		}
		return _sink;
	}
};
//--------------------------------------------------------------------------------------
// mediasession
//--------------------------------------------------------------------------------------
class event_serversession : public ServerMediaSession
{
	struct mediacontainer_stream : public stream
	{
		mediacontainer_stream(char const *src, event_serversession *ourserversession) :
					stream(ourserversession->_pid){}
		virtual ~mediacontainer_stream()
		{

		}
		virtual void start(struct stream *thiz)
		{

		}
		virtual unsigned registed_subsession()
		{
			return _mysubsessions.size();
		}
		virtual void detach_subsessions()
		{
			for(auto &it : _mysubsessions)
			{
				it->detach_buffer();
			}
		}
	};

	struct uvc_stream : public stream
	{
		uvc *_uvc;
		encoder *_enc;
		avattr _attr;
		uvc_stream(char const *src, event_serversession *ourserversession) :
			stream(ourserversession->_pid)
		{
			if(src &&
					strlen (src) > 4 &&
					src[0] == '/' &&
					src[1] == 'd' &&
					src[2] == 'e' &&
					src[3] == 'v')
			{
				_uvc = new uvc(src);

				_attr.set(avattr_key::frame_video, avattr_key::frame_video, 0, 0.0);
				_attr.set(avattr_key::width, avattr_key::width, _uvc->video_width(), 0.0);
				_attr.set(avattr_key::height, avattr_key::height, _uvc->video_height(), 0.0);
				_attr.set(avattr_key::fps, avattr_key::fps, _uvc->video_fps(), 0.0);
				_attr.set(avattr_key::bitrate, avattr_key::bitrate, 400000, 0.0);
				_attr.set(avattr_key::gop, avattr_key::gop, 10, 0.0);
				_attr.set(avattr_key::max_bframe, avattr_key::max_bframe, 1, 0.0);
				_attr.set(avattr_key::video_encoderid, avattr_key::video_encoderid, (int)AV_CODEC_ID_H264, 0.0);
				/*for h264 format pixed format*/
				_attr.reset(avattr_key::pixel_format, avattr_key::pixel_format, (int)AV_PIX_FMT_YUV420P, 0.0);
				_enc = new encoder(_attr);

				/*current uvc has only video frame*/
				_mysubsessions.push_back(new event_h264_servermediasubsession(ourserversession->envir(), (_pid << 16) | AVMEDIA_TYPE_VIDEO, this));
				ourserversession->addSubsession(_mysubsessions.back());
			}
		}
		virtual ~uvc_stream()
		{
			end();
			if(_uvc) delete _uvc;
			if(_enc) delete _enc;
		}
		virtual unsigned registed_subsession()
		{
			return _mysubsessions.size();
		}
		virtual void detach_subsessions()
		{
			for(auto &it : _mysubsessions)
			{
				it->detach_buffer();
			}
		}

		void operator()(avframe_class &frm, avpacket_class &pkt,void * )
		{
			/*
				put data to scheduler.
				called another thread
			*/
			for(auto &it : _mysubsessions)
			{
				it->send(_pid,
						AVMEDIA_TYPE_VIDEO,
						pkt.raw()->data,
						pkt.raw()->size);
			}
		}
		virtual void start(struct stream *thiz)
		{
			DECLARE_LIVEMEDIA_NAMEDTHREAD("rtsp server uvc source reader subsession");
			while(1)
			{
				/*
				 	 wake up when our eventsource calling 'dogetnextframe()'
				 */
				autolock a(_stream_lock);
				if(_bloop)
				{
					break;
				}

				int res = _uvc->waitframe(10000);
				/*
					 because enough time for get video,
					 return value invalid has mean that uvc error....
				 */
				DECLARE_THROW(res <= 0, "source uvc has broken");

				_uvc->get_videoframe([&](pixelframe &pix)->int{
						{
							swxcontext_class (pix, _attr);
						}
						_enc->encoding(pix,(*this));
					});
			}
		}
	};
	unsigned make_subsessions(const std::list<char const *> &srcs)
	{
		for(auto &it : srcs)
		{
			/*tell uvc*/
			{
				uvc_stream *p = new uvc_stream(it, this);
				if(p->registed_subsession() > 0)
				{
					streams.push_back(p);
					continue;
				}
				delete p;
			}
			/*tell mediacontainer*/
			{
				mediacontainer_stream *p = new mediacontainer_stream(it, this);
				if(p->registed_subsession() > 0)
				{
					streams.push_back(p);
					continue;
				}
				delete p;
			}
		}
		for(auto &it : streams)
		{
			it->start();
		}
		return streams.size();
	}
public:
	static ServerMediaSession *createnew(const std::list<char const *> &srcs,
				UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines)
	{
		event_serversession *newsession = new event_serversession(env, streamName, info, description, isSSM, miscSDPLines);
		DECLARE_THROW(newsession->make_subsessions(srcs) <= 0, "no found stream");
		return newsession;
	}
	event_serversession(UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines) :
		    	   ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines),
				   _pid(0) { }
	virtual ~event_serversession()
	{
		for(auto &it : streams)
		{
			/*
			 	 detach reading thread only
			 */
			it->end();
		}
		/*
		 	 detach streams first
		 */
		deleteAllSubsessions();
		for(auto &it : streams)
		{
			delete it;
		}
		streams.clear();
	}
	std::list<stream *> streams;
	/*like subsessions device source id*/
	int _pid;
};
}

