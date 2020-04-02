#pragma once


class live5rtspclient :
		public ::RTSPClient
{
public:
	class mediasubsession : public MediaSubsession
	{
		friend class live5rtspclient;
	private:
		char *_title;
		mediasubsession(MediaSession& parent) : MediaSubsession(parent),
				_title(nullptr){}
		  virtual ~mediasubsession()
		  {   delete[] _title; }
		  /*
			   no found "i" in live555 code... why ???
		   */
		  void parseSDPLine_i(char const* sdpLine)
		  {
				  // Check for "i=<media title>" line
				  char* buffer = strDupSize(sdpLine);

				  if (sscanf(sdpLine, "i=%[^\r\n]", buffer) == 1) {
					delete[] _title;
					_title = strDup(buffer);
				  }
				  delete[] buffer;
		  }

		  Boolean parseSDPLine_c(char const* sdpLine)
		  {
			  parseSDPLine_i(sdpLine);
			  return MediaSubsession::parseSDPLine_c(sdpLine);
		  }
	public:
		  bool hastitle() const { return _title != nullptr; }
		  char *strdup_title() const { return hastitle() ? strDup(_title) : nullptr;}
		  char const *ref_title() const {return _title;}
	};

	typedef enum
	{
		rtsp_option,
		rtsp_describe,
		rtsp_setup,
		rtsp_play,
		rtsp_pause,
		rtsp_resume,
		rtsp_max
	}rtsp_command;
	struct report
	{
		typedef std::function<void (void *ptr, live5rtspclient::mediasubsession const &)> except_subsession;
		typedef std::function<bool (void *ptr, live5rtspclient::mediasubsession const &)> setup_subsession;
		typedef std::function<void (void *ptr, live5rtspclient::mediasubsession const &)> bye_subsession;
		typedef std::function<void (void *ptr, live5rtspclient::mediasubsession const &, unsigned char *, unsigned, unsigned,
				  struct timeval, unsigned)> read_subsession;
		typedef std::function<void (void *ptr)> ready_session;
		typedef std::function<void (void *ptr)> except_session;
		void *_ptr;
		setup_subsession _setup_subsession;
		except_subsession _except_subsession;
		bye_subsession _bye_subsession;
		read_subsession _read_subsession;
		ready_session _ready_session;
		except_session _except_session;
                report() :
                    _ptr(nullptr),
                    _setup_subsession(nullptr),
                    _except_subsession(nullptr),
                    _bye_subsession(nullptr),
                    _read_subsession(nullptr),
                    _ready_session(nullptr),
                    _except_session(nullptr){}
		report(void *ptr,
				setup_subsession &&a,
				except_subsession &&b,
				bye_subsession &&c,
				read_subsession &&d,
				ready_session &&e,
				except_session &&f) :
				_ptr(ptr),
				_setup_subsession(a),
				_except_subsession(b),
				_bye_subsession(c),
				_read_subsession(d),
				_ready_session(e),
				_except_session(f){}
		bool operator()()
		{
			return _setup_subsession &&
					_except_subsession&&
					_bye_subsession&&
					_read_subsession&&
					_ready_session&&
					_except_session;
		}
	};
	/*
	 	 constructor
	 */
	live5rtspclient(UsageEnvironment& env,
			const report &notify,
			char const* rtspurl,
			char const *authid = nullptr,
			char const *authpwd = nullptr) :
				::RTSPClient(env, rtspurl, 10, nullptr, 0, -1),
				 _scheduledtoken(nullptr),
				 _auth(nullptr),
				 _session(nullptr),
				 _subsession(nullptr),
				 _iter(nullptr),
				 _notify(notify),
				 _duration(INFINITE)

	{
		if(authid && authpwd)
		{
			_auth = new Authenticator(authid, authpwd);
		}
		/*
		 	 start schedule 'option'
		 */
		sendcommand(rtsp_option);
	}
	virtual ~live5rtspclient()
	{
		reset_all_session();
	}
	void startplay()
	{
		sendcommand(rtsp_play);
	}
	void startpause()
	{
		sendcommand(rtsp_pause);
	}
	void startresume()
	{
		sendcommand(rtsp_resume);
	}
protected:
	/*
		 use for subsession's "i" flag
	 */
	class mediasession : public MediaSession
	{
		friend class live5rtspclient;
		mediasession(UsageEnvironment& env) : MediaSession(env){}
		  virtual ~mediasession(){}
		  Boolean initializeWithSDP(char const* sdpDescription)
		  {
			  return MediaSession::initializeWithSDP(sdpDescription);
		  }
		  static mediasession* createNew(UsageEnvironment& env,
						 char const* sdpDescription)
		  {
			  mediasession* session = nullptr;
			  do
			  {
				  session = new mediasession(env);
				  if(!session)
				  {
					  break;
				  }
				  if(!session->initializeWithSDP(sdpDescription))
				  {
					  delete session;
					  session  = nullptr;
					  break;
				  }
			  }while(0);
			  return session;
		  }
		  virtual MediaSubsession* createNewMediaSubsession()
		  { return new mediasubsession(*this); }
	};
	/*
	 	 create sink objects
	 */
	class mediasink : public MediaSink
	{
		friend class live5rtspclient;
	protected:
		const mediasubsession &_subsession;
		unsigned char *_buffer;
		_dword _buffersize;
		mediasink(UsageEnvironment &env, const mediasubsession &subsession, _dword buffersize) :
			MediaSink(env), _subsession(subsession), _buffersize(buffersize)
		{ if(_buffersize) _buffer = new unsigned char[_buffersize]; }
		virtual ~mediasink()
		{ delete [] _buffer; }
		virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
					  struct timeval presentationTime, unsigned durationInMicroseconds)
		{
			((live5rtspclient*)(_subsession.miscPtr))->_notify._read_subsession(((live5rtspclient*)(_subsession.miscPtr))->_notify._ptr, _subsession,
					_buffer,
					frameSize,
					numTruncatedBytes,
					presentationTime,
					durationInMicroseconds);
			continuePlaying();
		}
		virtual Boolean continuePlaying()
		{
			if(!fSource) return False;
			if(!_buffer) return False;
			fSource->getNextFrame(_buffer, _buffersize, [](void* a,
					unsigned b,
					unsigned c,
					struct timeval d,
					unsigned e)->void{((mediasink *)a)->afterGettingFrame(b,c,d,e);},
					this,
					onSourceClosure,
					this);
			return True;
		}


		static MediaSink *createNew(UsageEnvironment &env, const mediasubsession &subsession)
		{
			if(!subsession.mediumName() ||
					!subsession.codecName())
			{
				return nullptr;
			}
			if(subsession.mediumName()[0] == 'v' ||
					subsession.mediumName()[0] == 'V' )
			{
				if(!strcmp(subsession.codecName(), "H264"))
				{
					return new sink_h264(env, subsession);
				}
			}
			if(subsession.mediumName()[0] == 'a' ||
					subsession.mediumName()[0] == 'A' )
			{
				if(!strcmp(subsession.codecName(), "MPA"))
				{
					return new sink_mp3(env, subsession);
				}
				if(!strcmp(subsession.codecName(), "MPEG4-GENERIC"))
				{
					if(subsession.attrVal_strToLower("mode") &&
							!strcmp(subsession.attrVal_strToLower("mode"), "aac-hbr"))
					{

						return new sink_mpeg4_aac_hbr(env, subsession);
					}
				}
			}
			return nullptr;
		}
	};

	class sink_mpeg4_aac_hbr  : public mediasink
	{
		friend class live5rtspclient;
		sink_mpeg4_aac_hbr (UsageEnvironment &env,
				const mediasubsession &subsession) :
					mediasink(env, subsession, LIVE5_BUFFER_SIZE){}
		virtual ~sink_mpeg4_aac_hbr(){}
	};

	class sink_mp3  : public mediasink
	{
		friend class live5rtspclient;
		sink_mp3 (UsageEnvironment &env,
				const mediasubsession &subsession) :
					mediasink(env, subsession, LIVE5_BUFFER_SIZE){}
		virtual ~sink_mp3(){}
	};
	/*
	 	 sink 'H264'
	 */
	class sink_h264 : public mediasink
	{
		friend class live5rtspclient;
		bool _first;
		char const *_sprop;
		virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
					  struct timeval presentationTime, unsigned durationInMicroseconds)
		{

			if(!_first)
			{
				unsigned num;
				SPropRecord *record = NULL;
				u_int8_t nal;

				record = parseSPropParameterSets(_sprop, num);

				for(unsigned i = 0; i < num; i++)
				{
					if(record[i].sPropLength <= 0)
					{
						continue;
					}

					nal =  ((record[i].sPropBytes[0]) & 0x1F);
					if(nal == 7/*sps*/ ||
							nal == 8/*pps*/)
					{
						_dword datasize = 0;
						datasize = 4/*start code 0x00, 0x00, 0x00, 0x01*/ + record[i].sPropLength;/*sps length*/
						unsigned char pdata[datasize];
						pdata[0] = 0x00;
						pdata[1] = 0x00;
						pdata[2] = 0x00;
						pdata[3] = 0x01;


						memcpy(pdata + 4, record[i].sPropBytes, record[i].sPropLength);
						((live5rtspclient*)(_subsession.miscPtr))->_notify._read_subsession(((live5rtspclient*)(_subsession.miscPtr))->_notify._ptr, _subsession,
								pdata,
								datasize,
								0,
								presentationTime,
								durationInMicroseconds);
					}
				}
				delete [] record;
				_first = true;
			}
			unsigned char incomData[frameSize + 4];
			memset(incomData, 0, frameSize + 4);
			incomData[0] = 0x00;
			incomData[1] = 0x00;
			incomData[2] = 0x00;
			incomData[3] = 0x01;
			memcpy(incomData+4, _buffer, frameSize);
			((live5rtspclient*)(_subsession.miscPtr))->_notify._read_subsession(((live5rtspclient*)(_subsession.miscPtr))->_notify._ptr, _subsession,
					incomData,
					frameSize+4,
					numTruncatedBytes,
					presentationTime,
					durationInMicroseconds);
			continuePlaying();
		}
		sink_h264(UsageEnvironment &env, const mediasubsession &subsession) : mediasink(env, subsession, LIVE5_BUFFER_SIZE),
		_first(false), _sprop(subsession.fmtp_spropparametersets()){}
		virtual ~sink_h264(){}
	};
	TaskToken _scheduledtoken;
	Authenticator *_auth;
	mediasession *_session;
	mediasubsession *_subsession;
	MediaSubsessionIterator *_iter;
	report _notify;
	unsigned _duration;
	unsigned _duration_chk;



        virtual void response_option(int code, char* str)
	{
		do
		{
			if(code != 0)
			{
				printf("%s\n", str);
				break;
			}
			sendcommand(rtsp_describe);
		}while(0);
		delete [] str;
	}
        virtual void response_describe(int code, char* str)
	{
		do
		{
			if(code != 0)
			{
				printf("%s\n", str);
				break;
			}
			_session = mediasession::createNew(envir(), str);
			if(!_session)
			{

				break;
			}
			_iter = new MediaSubsessionIterator(dynamic_cast<MediaSession const &>(*_session));
			if(!_iter)
			{
				break;
			}
			while((_subsession = (mediasubsession *)_iter->next()) != nullptr)
			{
				if(!_notify._setup_subsession(_notify._ptr, *_subsession))
				{
					continue;
				}
				if(!_subsession->initiate())
				{
					_notify._except_subsession(_notify._ptr, *_subsession);
					continue;
				}
				_subsession->sink = mediasink::createNew(envir(), *_subsession);
				if(!_subsession->sink)
				{
					_notify._except_subsession(_notify._ptr, *_subsession);
					continue;
				}
				sendcommand(rtsp_setup);
				break;
			}
		}while(0);

		delete [] str;
	}
        virtual void response_setup(int code, char* str)
	{
		/*
		 	 setup subseesion
		 */
		do
		{
			if(code != 0)
			{
				printf("%s\n", str);
				_notify._except_subsession(_notify._ptr, *_subsession);
				if(_subsession->sink)Medium::close(_subsession->sink);
				_subsession->sink = nullptr;
				break;
			}

			_subsession->miscPtr = (void *)this;
			if( _subsession->rtcpInstance())
			{
				_subsession->rtcpInstance()->setByeHandler([](void *ptr)->void{
					((live5rtspclient *)(((mediasubsession *)ptr)->miscPtr))->subsession_bye(ptr);
				}, _subsession);
			}

		}while(0);

		delete [] str;
		/*
				find next subsession
		 */
		while((_subsession = (mediasubsession *)_iter->next()) != nullptr)
		{
			if(!_notify._setup_subsession(_notify._ptr, *_subsession))
			{
				continue;
			}
			if(!_subsession->initiate())
			{
				_notify._except_subsession(_notify._ptr, *_subsession);
				continue;
			}
			_subsession->sink = mediasink::createNew(envir(), *_subsession);
			if(!_subsession->sink)
			{
				_notify._except_subsession(_notify._ptr, *_subsession);
				continue;
			}
			sendcommand(rtsp_setup);
			return;
		}

		/*
				if no more subsession find, start play
		 */
		MediaSubsessionIterator iter(*_session);
		mediasubsession *ptr = nullptr;
		while((ptr = ((mediasubsession *)iter.next())) != nullptr)
		{
			if(ptr->sink)
			{
				_notify._ready_session(_notify._ptr);
				break;
			}
		}
	}
        virtual void response_play(int code, char* str)
	{
		if(code != 0)
		{
			printf("%s\n", str);
			delete [] str;
			_notify._except_session(_notify._ptr);
			return;
		}
		double session_duration = 0.0;

		session_duration = _session->playStartTime() - _session->playEndTime();
		if(session_duration > 0)
		{
			_duration = (unsigned )(session_duration * 1000);
			_duration_chk = sys_time_c();
			if(_scheduledtoken)
			{
				envir().taskScheduler().unscheduleDelayedTask(_scheduledtoken);
				_scheduledtoken = nullptr;
			}
			_scheduledtoken = envir().taskScheduler().scheduleDelayedTask(_duration * 1000, [](void *ptr)->void{
				((live5rtspclient *)ptr)->overtime_stream();
			}, this);
		}

		MediaSubsessionIterator iter(*_session);
		mediasubsession *ptr = nullptr;
		while((ptr = ((mediasubsession *)iter.next())) != nullptr)
		{
			if(ptr->sink)
			{
				ptr->sink->startPlaying(*ptr->readSource(), [](void *ptr)->void{
					((live5rtspclient *)(((mediasubsession *)ptr)->miscPtr))->subsession_bye(ptr);
				}, ptr);
			}
		}
		delete [] str;
	}
	virtual void response_pause(int code, char *str)
	{
		if(code != 0)
		{
			printf("can't allow method pause \n%s\n", str);
			delete [] str;
			/*
			 	 no puase is not except
			 */
		//	_notify._except_session(_notify._ptr);
			return;
		}
		if(_scheduledtoken)
		{
			envir().taskScheduler().unscheduleDelayedTask(_scheduledtoken);
			_scheduledtoken = nullptr;
		}
		if(_duration != INFINITE)
		{
			_duration -=  sys_time_c() - _duration_chk;
		}


		MediaSubsessionIterator iter(*_session);
		mediasubsession *ptr = nullptr;
		while((ptr = ((mediasubsession *)iter.next())) != nullptr)
		{
			if(ptr->sink)
			{
				ptr->sink->stopPlaying();
			}
		}
		delete [] str;
	}
	virtual void response_resume(int code, char *str)
	{
		if(code != 0)
		{
			printf("can't allow method resume\n%s\n", str);
			delete [] str;
			/*
				 no resume is not except
			 */
					//	_notify._except_session(_notify._ptr);
			//_notify._except_session(_notify._ptr);
			return;
		}
		if(_scheduledtoken)
		{
			envir().taskScheduler().unscheduleDelayedTask(_scheduledtoken);
			_scheduledtoken = nullptr;
		}
		if(_duration != INFINITE)
		{
			_duration_chk = sys_time_c();
			_scheduledtoken = envir().taskScheduler().scheduleDelayedTask(_duration * 1000, [](void *ptr)->void{
				((live5rtspclient *)ptr)->overtime_stream();
			}, this);
		}

		MediaSubsessionIterator iter(*_session);
		mediasubsession *ptr = nullptr;
		while((ptr = ((mediasubsession *)iter.next())) != nullptr)
		{
			if(ptr->sink)
			{
				ptr->sink->startPlaying(*ptr->readSource(), [](void *ptr)->void{
					((live5rtspclient *)(((mediasubsession *)ptr)->miscPtr))->subsession_bye(ptr);
				}, ptr);
			}
		}
		delete [] str;
	}
	void subsession_bye(void *ptr)
	{

		mediasubsession *subsession = (mediasubsession *)ptr;
		_notify._bye_subsession(_notify._ptr, *subsession);
		if(subsession->sink)
		{
			Medium::close(subsession->sink);
		}
		subsession->sink = nullptr;


		MediaSubsessionIterator iter(*_session);
		mediasubsession *subsession_remain = nullptr;
		while((subsession_remain = ((mediasubsession *)iter.next())) != nullptr)
		{
			if(subsession_remain->sink)
			{
				return;
			}
		}
		reset_all_session();
	}
	void overtime_stream()
	{
		reset_all_session();
	}

	void reset_all_session()
	{
		if(_scheduledtoken)
		{
			envir().taskScheduler().unscheduleDelayedTask(_scheduledtoken);
			_scheduledtoken = nullptr;
		}
		if(_iter)
		{
			delete _iter;
			_iter = nullptr;
		}
		if(_session)
		{

			bool _hasactive_session = false;
			MediaSubsessionIterator iter(*_session);
			mediasubsession *subsession_remain = nullptr;
			while((subsession_remain = ((mediasubsession *)iter.next())) != nullptr)
			{
				if(subsession_remain->sink)
				{
					_hasactive_session = true;
					Medium::close(subsession_remain->sink);
					subsession_remain->sink = nullptr;
					_notify._bye_subsession(_notify._ptr, *subsession_remain);
					if(subsession_remain->rtcpInstance())
					{
						subsession_remain->rtcpInstance()->setByeHandler(NULL, NULL);
					}
				}
			}

			if(_hasactive_session)
			{
				RTSPClient::sendTeardownCommand(*_session, nullptr, _auth);
			}
			Medium::close(_session);
			_session = nullptr;
		}
		char *ptr = strDup(url());
		RTSPClient::reset();
		setBaseURL(ptr);
		delete [] ptr;
	}
	void sendcommand(rtsp_command cmd)
	{
		if(cmd == rtsp_option)
			RTSPClient::sendOptionsCommand(
					[](RTSPClient* a, int b, char* c) ->void
					{
						((live5rtspclient *)a)->response_option(b, c);
					},
					_auth);
		else if(cmd == rtsp_describe)
			RTSPClient::sendDescribeCommand(
					[](RTSPClient* a, int b, char* c) ->void{
					(
						(live5rtspclient *)a)->response_describe(b, c);
					},
					_auth);
		else if(cmd == rtsp_setup)
			RTSPClient::sendSetupCommand(dynamic_cast<MediaSubsession &>(*_subsession),
					[](RTSPClient* a, int b, char* c) ->void
					{
						((live5rtspclient *)a)->response_setup(b, c);
					},
					False,
					False,
					False,
					_auth);
		else if(cmd == rtsp_play)
		{
			if(_session->absEndTime())
				RTSPClient::sendPlayCommand(dynamic_cast<MediaSession &>(*_session),
						[](RTSPClient* a, int b, char* c) ->void
						{
							((live5rtspclient *)a)->response_play(b, c);
						},
						_session->absStartTime(),
						_session->absEndTime(),
						1.0f,
						_auth);
			else
				RTSPClient::sendPlayCommand(dynamic_cast<MediaSession &>(*_session),
						[](RTSPClient* a, int b, char* c) ->void
						{
							((live5rtspclient *)a)->response_play(b, c);
						},
						0.0f,
						-1.0f,
						1.0f,
						_auth);
		}
		else if(cmd == rtsp_pause)
		{
			RTSPClient::sendPauseCommand(dynamic_cast<MediaSession &>(*_session),
					[](RTSPClient* a, int b, char* c) ->void
					{
						((live5rtspclient *)a)->response_pause(b, c);
					},
					_auth);
		}
		else if(cmd == rtsp_resume)
		{

			RTSPClient::sendPlayCommand(dynamic_cast<MediaSession &>(*_session),
					[](RTSPClient* a, int b, char* c) ->void
					{
						((live5rtspclient *)a)->response_resume(b, c);
					},
					-1.0f,
					-1.0f,
					1.0f,
					_auth);
		}
	}

};
