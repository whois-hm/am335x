#pragma once

class rtsp_playback :
		public playback_inst
{
	enum  rtsp_playback_state
	{
		rtsp_playback_state_open = 0,
		rtsp_playback_state_pause_from_open,
		rtsp_playback_state_pause_from_run,
		rtsp_playback_state_run,
		rtsp_playback_state_close,
		rtsp_playback_state_max
	};
	struct pixelframe_pts :
			public pixelframe_presentationtime
	{
		pixelframe_pts(const pixelframe &f,
				double normalplaytime) :
			pixelframe_presentationtime(f)
			{_pts = normalplaytime;}
		virtual ~pixelframe_pts(){}
		virtual double getpts()
		{
			return _pts;
		}
		void operator()()
		{
			/*
			 	 already filed value pts
			 */
		}
	};

	struct pcmframe_pts :
			public pcmframe_presentationtime
	{
		double _copypts;
		pcmframe_pts(const pcmframe &f,
				double normalplaytime) :
			pcmframe_presentationtime(f),
			_copypts(normalplaytime)
		{_pts = normalplaytime;}
		virtual ~pcmframe_pts(){}
		virtual double getpts()
		{
			return _pts;
		}
		void operator()()
		{
			/*
				 already filed value pts
			 */
		}
		double guesspts()
		{
			return _copypts;
		}
	};

	struct framescheduler :
			public avframescheduler
			<pixelframe_pts, pcmframe_pts>
	{
		framescheduler(const avattr &attr) :
			avframescheduler(attr){ }
		virtual ~framescheduler(){}
		virtual void usingframe( pixelframe_pts &rf)
		{ rf(); /*unnecessary function calls*/}
		virtual void usingframe( pcmframe_pts &rf)
		{ rf(); /*unnecessary function calls*/}
	};


	class rtsppacket : public avpacket_class
	{
		/*
		 	 redefiniton packet class
		 */
	public:
		/*
		 	 saved live5 values
		 */
		unsigned _truncatedbyte;
		struct timeval _presentationtime;
		unsigned _durationinmicroseconds;
		double _normalplaytime;
	public:
		rtsppacket(unsigned char *data,  unsigned size, unsigned truncatedbyte,
				  struct timeval presentationtime, unsigned durationinmicroseconds,
				  double normalplaytime) :
					  avpacket_class(),
					  _truncatedbyte (truncatedbyte),
					  _presentationtime(presentationtime),
					  _durationinmicroseconds(durationinmicroseconds),
					  _normalplaytime(normalplaytime)
		{
			DECLARE_THROW(av_new_packet(&_pkt, size), "can't av_new_packet");
			memcpy(_pkt.data, data, size);
		}
		virtual ~rtsppacket()
		{ }
		rtsppacket(rtsppacket const &rhs) :
			avpacket_class(dynamic_cast<avpacket_class const &>(rhs)),
			_truncatedbyte(rhs._truncatedbyte),
			_presentationtime(rhs._presentationtime),
			_durationinmicroseconds(rhs._durationinmicroseconds),
			_normalplaytime(rhs._normalplaytime){ }
		rtsppacket const &operator = (rtsppacket const &rhs)
		{
			avpacket_class::operator =(dynamic_cast<avpacket_class const &>(rhs));
			_truncatedbyte = rhs._truncatedbyte;
			_presentationtime = rhs._presentationtime;
			_durationinmicroseconds = rhs._durationinmicroseconds;
			_normalplaytime = rhs._normalplaytime;
			return *this;
		}
		rtsppacket *clone() const
		{
			return new rtsppacket(*this);
		}
	};



	class stream :
			public ::decoder
	{
	public:
		stream() : decoder(),
		_attrs(nullptr),
		_type(AVMEDIA_TYPE_UNKNOWN),
		_framebuffer(nullptr),
		_framelock(nullptr){}/*use for opentest*/
		virtual ~stream(){}
		enum functor_event { functor_event_open_test, functor_event_put_packet, functor_event_close };
		struct fucntor_par
		{
			enum functor_event e;
			union
			{
				struct
				{
					enum AVCodecID codec;
					enum AVMediaType regtype;
					std::pair<avattr, avattr> *regattr;
					framescheduler *regframebuffer;
					std::mutex *regframelock;
					bool *res;
				} opentest;
				struct
				{
					rtsppacket *pkt;
				}putpacket;
			};
		};
		bool operator()(_dword r, void *par, _dword size, _dword *qtime)
		{
			if(r == WQOK)
			{
				stream:: fucntor_par *param = (stream:: fucntor_par *)par;
				if(param->e == functor_event_open_test)
				{
					decoder_open_test(param);
					return true;
				}
				if(param->e == functor_event_put_packet)
				{
					put_packet(param);
					return true;
				}
			}
			return false;
		}
		void operator()(const avpacket_class & pkt,
				avframe_class &frm,
				void *puser)
		{
			if(_type == AVMEDIA_TYPE_VIDEO)
			{
				pixelframe_pts  _pixelframe(*frm.raw(),
						dynamic_cast<const rtsppacket &>(pkt)._normalplaytime);

				swxcontext_class (dynamic_cast<pixelframe &>(_pixelframe),
						_attrs->second);

				autolock a(*_framelock);
				*_framebuffer << (_pixelframe);
			}
			else if(_type == AVMEDIA_TYPE_AUDIO)
			{

				pcmframe_pts  _pcmframe(*frm.raw(),
						dynamic_cast<const rtsppacket &>(pkt)._normalplaytime);

				swxcontext_class (dynamic_cast<pcmframe &>(_pcmframe) ,
						_attrs->second);

				autolock a(*_framelock);
				*_framebuffer << (_pcmframe);
			}

		}
	private:

		void put_packet(stream::fucntor_par *par)
		{
			decoding(dynamic_cast<avpacket_class &>(*par->putpacket.pkt),
					*this);
			delete par->putpacket.pkt;
		}
		void decoder_open_test(stream::fucntor_par *par)
		{
			if(opentest(par->opentest.regattr->first, par->opentest.codec))
			{
				_attrs = par->opentest.regattr;
				_type = par->opentest.regtype;
				_framebuffer = par->opentest.regframebuffer;
				_framelock = par->opentest.regframelock;
				*par->opentest.res = true;

			}
		}
		/*
		 	 registed value from 'streamer' when decoder open test.
		 	 we sure that this value always valid, because streamer contain this class.
		 */
		std::pair<avattr, avattr> *_attrs;
		enum AVMediaType _type;
		framescheduler *_framebuffer;
		std::mutex *_framelock;

	};
	class streamer:
			public wthread<stream>
	{


		/*
		 	 because for speed,  we should maintain this class until close client,
		 */
	private:
		/*our attribute first input for decoder second for output*/
		std::pair<avattr, avattr> _attrs;
		/*media subsessions tag 'm'*/
		enum AVMediaType _mediatype;
		enum AVCodecID _codecid;

		framescheduler *_framebuffer;

		/*set if known media subsessions tag 'i'*/
		std::string live5medium;
		std::string live5codecid;
		std::string live5title;
		std::mutex _accesslock;
		std::mutex _framelock;
		std::mutex _pauselock;
	public:
		streamer const &operator = (streamer const &) =  delete;
		streamer(const streamer &rhs) = delete;
		streamer(const avattr &attr_input,
				const avattr &attr_output,
				enum AVMediaType mediatype,
				enum AVCodecID codecid,
				char const *medium,
				char const *codec,
				char const *title) :
			wthread(10, sizeof(stream::fucntor_par), std::string(std::string(medium) + std::string(codec)).c_str()),
			_attrs(std::make_pair(attr_input, attr_output)),
			_mediatype(mediatype),
			_codecid(codecid),
			_framebuffer(new framescheduler(attr_output)),
			live5medium(medium),
			live5codecid(codec),
			live5title(title ? title : "")
		{
			/*
			 	 start self
			 */
			start(INFINITE);
		}
		virtual ~streamer()
		{
			{
				autolock a(_accesslock);
				if(exec())
				{
					stream::fucntor_par par;
					par.e = stream::functor_event_close;
					sendto(&par, sizeof(stream::fucntor_par));
					end();
				}

			}
		}
		enum AVMediaType type()
		{
			return _mediatype;
		}
		void put_packet(rtsppacket const &pkt)
		{
			{
				autolock a(_accesslock);
				if(!exec())
				{
					return;
				}
			}


//			unsigned mspts = (pkt._presentationtime.tv_sec * 1000) + (pkt._presentationtime.tv_usec / 1000);
//			printf("%s durationmicroseconds : %d, presentation time : %d normal playtime : %f\n", _mediatype == AVMEDIA_TYPE_VIDEO ? "video" : "audio", pkt._durationinmicroseconds, pkt._presentationtime.tv_usec, pkt._normalplaytime);


			stream::fucntor_par par;
			par.e = stream::functor_event_put_packet;
			par.putpacket.pkt =pkt.clone();

			sendto(&par, sizeof(stream::fucntor_par));
		}
		bool stream_decoder_open_test()
		{
			{
				autolock a(_accesslock);
				if(!exec())
				{
					return false;
				}
			}

			bool res = false;
			stream::fucntor_par par;
			par.e = stream::functor_event_open_test;
			par.opentest.codec = _codecid;
			par.opentest.regattr = &_attrs;
			par.opentest.regtype = _mediatype;
			par.opentest.regframebuffer = _framebuffer;
			par.opentest.regframelock = &_framelock;
			par.opentest.res = &res;
			sendto_wait(&par, sizeof(stream::fucntor_par));
			return res;
		}
		bool match_title(char const *title)
		{
			if(!title)
			{
				return false;
			}
			if(live5title.empty())
			{
				return false;
			}
			return live5title == std::string(title);
		}
		bool match_from_subsession(live5rtspclient::mediasubsession const &subsession)
		{
			if(!live5title.empty() &&
					subsession.ref_title())
			{
				return live5title == subsession.ref_title();
			}
			return live5medium == subsession.mediumName() &&
					live5codecid == subsession.codecName();
		}
		void shutdown()
		{
			printf("shutdown....\n");
			autolock a(_accesslock);
			if(exec())
			{
				stream::fucntor_par par;
				par.e = stream::functor_event_close;
				sendto(&par, sizeof(stream::fucntor_par));

				end();
			}
		}
		bool has_shutdowned()
		{
			autolock a(_accesslock);
			return exec() ? false : true;
		}
		void pause_lock(bool bpause)
		{
			if(bpause)
			{
				_pauselock.lock();
			}
			else
			{
				_pauselock.unlock();
			}
		}
		template <class T>
		int get_frame(T &d)
		{
			/*
			 	 follow playback 'take'
				 if 0 > 1 take frame
				 or 0 no ready frame
				 or < 0 err
			 */
			autolock w(_pauselock);
			/*
				 if no syncronize 'exec' but we maintain this class
			 */
			if(has_shutdowned())
			{
				printf("get frame shutdown\n");
				return -1;
			}


			autolock a(_framelock);
			*_framebuffer >> (d);
			return d.can_take() ? 1 : 0;
		}
		int get_frame(pcm_require &d)
		{
			autolock w(_pauselock);
			if(has_shutdowned())
			{

				return -1;
			}


			autolock a(_framelock);
			*_framebuffer >> (d);
			return d.first.can_take() ? 1 : 0;
		}
	};



private:
	enum  rtsp_playback_state _state;
	unsigned _connection_timeout;
	live5scheduler <live5rtspclient>*_scheduler;
	semaphore _conwait_sema;
	std::pair<std::mutex , std::vector<streamer *>> _stremers;
	static const unsigned  int live5scheduler_rtsp_playback_startplay = live5scheduler<live5rtspclient>::live5scheduler_trigger_id_user_start;
	static const unsigned  int live5scheduler_rtsp_playback_startpause = live5scheduler_rtsp_playback_startplay + 1;
	static const unsigned  int live5scheduler_rtsp_playback_startresume= live5scheduler_rtsp_playback_startpause + 1;



	std::pair<enum AVMediaType, enum AVCodecID> matching_codec(live5rtspclient::mediasubsession const &subsession,
			avattr &subsession_attr)
	{
		/*matching value our supoorted ffmpeg value from live555 session sdpdescription */

		std::string str_type(subsession.mediumName());
		std::string str_codec(subsession.codecName());

		if(str_type == "video" &&
				str_codec == "H264")
		{
			subsession_attr.set(avattr_key::frame_video, "video subsession", 0,0);
			subsession_attr.set(avattr_key::width, "video subsession width", (int)subsession.videoWidth(),0.0);
			subsession_attr.set(avattr_key::height, "video subsession height", (int)subsession.videoHeight(),0.0);
			subsession_attr.set(avattr_key::pixel_format, "video subsession format", AV_PIX_FMT_YUV420P,0.0);

			return std::pair<enum AVMediaType, enum AVCodecID>(AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H264);
		}
		if(str_type == "audio")
		{
			if(str_codec == "MPA")
			{
				return std::pair<enum AVMediaType, enum AVCodecID>(AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_MP3);
			}
			if(str_codec == "MPEG4-GENERIC")
			{
				std::string mode = subsession.attrVal_strToLower("mode");
				if(!mode.empty() &&
						mode == "aac-hbr")
				{
					subsession_attr.set(avattr_key::frame_audio, "audio subsession", 0,0);
					subsession_attr.set(avattr_key::channel, "audio subsession channel", (int)subsession.numChannels(),0.0);
					subsession_attr.set(avattr_key::samplerate, "audio subsession samplerate", (int)subsession.rtpTimestampFrequency(),0.0);
					subsession_attr.set(avattr_key::pcm_format, "audio subsession format", AV_SAMPLE_FMT_S16,0.0);
					return std::pair<enum AVMediaType, enum AVCodecID>(AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC);
				}
			}
		}

		printf("rtspclient can't support media : %s codec : %s\n", str_type.c_str() ? str_type.c_str() : "unknown", str_codec.c_str() ? str_codec.c_str() : "unknown");
		return std::pair<enum AVMediaType, enum AVCodecID>(AVMEDIA_TYPE_UNKNOWN, AV_CODEC_ID_NONE);
	}
	template <typename _mat_functor,
	typename _and_functor>
	void finder(_mat_functor &&_mf,
			_and_functor &&_af)
	{
		streamer *t = nullptr;
		{
			autolock a(_stremers.first);
			for(auto &it : _stremers.second)
			{
				if(_mf(it)) { t = it; break; }
			}
		}
		if(t) _af(t);
	}
	template<typename _functor>
	void find_and(live5rtspclient::mediasubsession const &subsession, _functor &&_f)
	{
		finder([&](streamer *t)->bool{return t->match_from_subsession(subsession);},
				_f);
	}
	template<typename _functor>
	void find_and(enum AVMediaType type, _functor &&_f)
	{
		finder([&](streamer *t)->bool{return t->type() == type;},
				_f);
	}
	template<typename _functor>
	void find_and(char const *title, _functor &&_f)
	{
		finder([&](streamer *t)->bool{return t->match_title(title);},
				_f);
	}
	template<typename _functor>
	void find_and(char const *title, enum AVMediaType type, _functor &&_f)
	{
		finder([&](streamer *t)->bool { return t->match_title(title) ? true : t->match_title(title) ; },
				std::forward(_f));
	}
	bool reported_from_client_setup_subsession(live5rtspclient::mediasubsession const &subsession)
	{
		streamer *new_streamer = nullptr;
		avattr inattr;

		std::pair<enum AVMediaType, enum AVCodecID> value ( matching_codec(subsession, inattr));
		printf("setup session %s %s trying\n", subsession.mediumName(), subsession.codecName());
		if(value.first == AVMEDIA_TYPE_UNKNOWN)
		{
			printf("setup session type unknown\n");
			return false;
		}
		if(value.first == AVMEDIA_TYPE_VIDEO &&
				!_attr.has_frame_video())
		{
			printf("setup session video user ignore\n");
			return false;
		}
		if(value.first == AVMEDIA_TYPE_AUDIO &&
				!_attr.has_frame_audio())
		{
			printf("setup session audio user ignore\n");
			return false;
		}
		new_streamer = new streamer(inattr,
				_attr,
				value.first,
				value.second,
				subsession.mediumName(),
				subsession.codecName(),
				subsession.ref_title());
		DECLARE_THROW(!new_streamer, "can't create streamer");


		if(!new_streamer->stream_decoder_open_test())
		{
			delete new_streamer;
			printf("decoder open test fail\n");
			return false;
		}

		autolock a(_stremers.first);
		printf("setup session %s %s ok!\n", subsession.mediumName(), subsession.codecName());
		_stremers.second.push_back(new_streamer);

		return true;
	}
	void reported_from_client_except_subsession(live5rtspclient::mediasubsession const &subsession)
	{
		find_and(subsession, [](streamer *s)->void {
			s->shutdown();
		});

	}
	void reported_from_client_bye_subsession(live5rtspclient::mediasubsession const &subsession)
	{
		find_and(subsession, [](streamer *s)->void {
			s->shutdown();
		});
	}
	void reported_from_client_read_subsession(live5rtspclient::mediasubsession const &subsession, unsigned char *data,  unsigned frameSize, unsigned numTruncatedBytes,
			  struct timeval presentationTime, unsigned durationInMicroseconds)
	{

		find_and(subsession, [&](streamer *s)->void {
			s->put_packet(rtsppacket(data,
					frameSize,
					numTruncatedBytes,
					presentationTime,
					durationInMicroseconds,
					const_cast<live5rtspclient::mediasubsession &>(subsession).getNormalPlayTime(presentationTime)));
		});
	}
	void reported_from_client_ready_session()
	{
		SEMA_unlock(&_conwait_sema);
	}
	void reported_from_client_except_session()
	{
		autolock a(_stremers.first);
		for(auto &it : _stremers.second)
		{
			it->shutdown();
		}
	}

public:
	rtsp_playback(const avattr &attr,
			char const *url,
			unsigned connectiontime,
			char const *auth_id = nullptr,
			char const *auth_pwd = nullptr) :
				playback_inst(attr),
				_state(rtsp_playback_state_open),
				_connection_timeout(connectiontime) ,
				_scheduler(nullptr)

	{

		live5rtspclient::report _report(
				(void *)this,
				[](void *ptr, live5rtspclient::mediasubsession const & subsession)->bool
				{return  ((rtsp_playback *)ptr)->reported_from_client_setup_subsession(subsession); },
				[](void *ptr, live5rtspclient::mediasubsession const & subsession)->void
				{ ((rtsp_playback *)ptr)->reported_from_client_except_subsession(subsession); },
				[](void *ptr, live5rtspclient::mediasubsession const & subsession)->void
				{ ((rtsp_playback *)ptr)->reported_from_client_bye_subsession(subsession); },
				[](void *ptr, live5rtspclient::mediasubsession const & subsession,
						unsigned char *data,
						unsigned datasize,
						unsigned numTruncatedBytes,
						struct timeval presentationTime,
						unsigned durationInMicroseconds)->void
				{ ((rtsp_playback *)ptr)->reported_from_client_read_subsession(subsession, data, datasize, numTruncatedBytes, presentationTime, durationInMicroseconds); },
				[](void *ptr)->void
				{ ((rtsp_playback *)ptr)->reported_from_client_ready_session(); },
				[](void *ptr)->void
				{ ((rtsp_playback *)ptr)->reported_from_client_except_session(); });


		_scheduler = new live5scheduler<live5rtspclient>();
		DECLARE_THROW(!_scheduler, "can't create scheduler");

		DECLARE_THROW(WQOK!= SEMA_open(&_conwait_sema, 0, 1), "can't open semaphore");
		/*
		 	 register event for rtsp play command send, when play is ready
		 */
		_scheduler->register_trigger(live5scheduler_rtsp_playback_startplay,
				[](	live5scheduler<live5rtspclient> *scheduler , void *ptr)->void {
					scheduler->refcallable()->startplay(); }, nullptr);

		_scheduler->register_trigger(live5scheduler_rtsp_playback_startpause,
				[](	live5scheduler<live5rtspclient> *scheduler , void *ptr)->void {
					scheduler->refcallable()->startpause(); }, nullptr);

		_scheduler->register_trigger(live5scheduler_rtsp_playback_startresume,
				[](	live5scheduler<live5rtspclient> *scheduler , void *ptr)->void {
					scheduler->refcallable()->startresume(); }, nullptr);

		_scheduler->start(true,/*start in new thread*/
				_report,
				url,
				auth_id,
				auth_pwd);

		pause();
	}
	virtual ~rtsp_playback()
	{
		SEMA_close(&_conwait_sema);
		if(_scheduler)
		{
			delete _scheduler;
		}
		/*
		 	 when thread has close, reporting value all cleared
		 	 (this mean when thread has close, class  'streamer' has all closed, but not deleted)
		 */
		_scheduler = nullptr;
		for(auto &it : _stremers.second)
		{
			delete it;
		}
		_stremers.second.clear();
	}
	virtual bool connectionwait()
	{
		if(SEMA_lock(&_conwait_sema, _connection_timeout) != WQ_SIGNALED)
		{
			/*
			 	 when thread has close, reporting value all cleared
			 	 (this mean when thread has close, class  'streamer' has all closed, but not deleted)
			 */
			_state = rtsp_playback_state_close;
			if(_scheduler)
			{
				delete _scheduler;
				_scheduler = nullptr;
			}
			return false;
		}
		return true;
	}
	void record(char const *file)
	{

	}
	virtual void pause()
	{
		if(_state == rtsp_playback_state_open)
		{
			_state = rtsp_playback_state_pause_from_open;
		}
		else if(_state == rtsp_playback_state_run)
		{
			_state = rtsp_playback_state_pause_from_run;
			if(_scheduler)
			{
				_scheduler->trigger(live5scheduler_rtsp_playback_startpause);
			}
		}
		else return;

		for(auto &it : _stremers.second)
		{
			it->pause_lock(true);
		}
	}
	virtual void resume(bool closing = false)
	{
		struct resume_table
		{
			bool bplay;
			bool bresume;
			bool unlock;
			enum  rtsp_playback_state next;
		};

		static struct resume_table table[rtsp_playback_state_max][2] =
		{
				{
						{true, false, false, rtsp_playback_state_pause_from_run},/*state = open, closing = false*/
						{false, false, false, rtsp_playback_state_close},/*state = open, closing = true*/
				},
				{
						{true, false, true, rtsp_playback_state_run},/*state = pause_from_open, closing = false*/
						{false, false, true, rtsp_playback_state_close},/*state = pause_from_open, closing = true*/
				},
				{
						{false, true, true, rtsp_playback_state_run},/*state = pause_from_run, closing = false*/
						{false, false, true, rtsp_playback_state_close},/*state = pause_from_run, closing = true*/
				},
				{
						{false, false, false, rtsp_playback_state_run},/*state = run, closing = false*/
						{false, false, false, rtsp_playback_state_close},/*state = run, closing = true*/
				},
				{
						{false, false, false, rtsp_playback_state_close},/*state = close, closing = false*/
						{false, false, false, rtsp_playback_state_close},/*state = close, closing = true*/
				}
		};
		resume_table what = table[_state][closing ? 1 : 0];
		if(what.bplay)
		{
			play();
		}
		if(what.bresume)
		{
			if(_scheduler)
			{
				_scheduler->trigger(live5scheduler_rtsp_playback_startresume);
			}
		}
		if(what.unlock)
		{
			for(auto &it : _stremers.second)
			{
				it->pause_lock(false);
			}
		}
		_state = what.next;
	}
	virtual void seek(double incr)
	{

	}
	virtual bool has(avattr::avattr_type_string &&key)
	{
		bool bfound = false;
		enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
		find_and(key.c_str(), [&](streamer *t)->void{
			bfound = true;
		});

		if(!bfound)
		{
			if(key == avattr_key::frame_video)
			{
				type = AVMEDIA_TYPE_VIDEO;
			}
			if(key == avattr_key::frame_audio)
			{
				type = AVMEDIA_TYPE_AUDIO;
			}
			if(type != AVMEDIA_TYPE_UNKNOWN)
			{
				find_and(type, [&](streamer *t)->void{
					bfound = true;
				});
			}
		}
		return bfound;
	}
	virtual void play()
	{
		if(_state == rtsp_playback_state_open ||
				_state == rtsp_playback_state_pause_from_open)
		{
			if(_scheduler)
			{
				_state = rtsp_playback_state_run;
				_scheduler->trigger(live5scheduler_rtsp_playback_startplay);
			}
		}
	}
        virtual duration_div duration()
        {
            return duration_div();
        }
	virtual int take(const std::string &title, pixel &output)
	{
		/*
		 	 we first found from 'title' in 'AVMediaTypeVideo'
		 	 if not found,  take 'AVMediaTypeVideo'
		 */
		bool bfound = false;
		int ret = -1;
		if(!title.empty())
		{
			find_and(title.c_str(), [&](streamer *t)->void{
				bfound = true;
				ret = t->get_frame(output);
				/*
				 	 other state manage in streamer
				 */
				if(_state ==rtsp_playback_state_close)
				{
					ret = -1;
				}
			});
		}

		if ( !bfound )
		{
			find_and(AVMEDIA_TYPE_VIDEO, [&](streamer *t)-> void{
						ret = t->get_frame(output);
						if(_state ==rtsp_playback_state_close)
						{
							ret = -1;
						}
					});
		}
		return ret;
	}
	virtual int take(const std::string &title, pcm_require &output)
	{
		/*
			 we first found from 'title' in 'AVMediaTypeAudio'
			 if not found,  take 'AVMediaTypeAudio'
		 */
		bool bfound = false;
		int ret = -1;
		if(!title.empty())
		{
			find_and(title.c_str(), [&](streamer *t)->void{
				bfound = true;
				ret = t->get_frame(output);
				if(_state ==rtsp_playback_state_close)
				{
					ret = -1;
				}
			});
		}
		if ( !bfound )
		{
			find_and(AVMEDIA_TYPE_AUDIO, [&](streamer *t)->void{
							ret = t->get_frame(output);
							if(_state ==rtsp_playback_state_close)
							{
								ret = -1;
							}
						});
		}
		return ret;
	}
};

