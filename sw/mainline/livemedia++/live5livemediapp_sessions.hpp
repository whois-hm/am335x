#pragma once

/*--------------------------------------------------------------------------------
 our server session
 --------------------------------------------------------------------------------*/
class livemediapp_serversession : public ServerMediaSession
{
	/*--------------------------------------------------------------------------------
	 our server media subsessions
	 --------------------------------------------------------------------------------*/

class livemediapp_servermediasubession :
		public OnDemandServerMediaSubsession
{
		/*based subsession*/
protected:
	FramedSource *_source;
	FramedSource *_filtersource;

	livemediapp_servermediasubession(UsageEnvironment &env, FramedSource *filteredsource) :
		OnDemandServerMediaSubsession(env, True/*reuse source*/), _source(nullptr), _filtersource(filteredsource) {}
	virtual ~livemediapp_servermediasubession()
	{
		if(_source)			 { Medium::close(_source); }
	}
	virtual void closeStreamSource(FramedSource *inputSource)
	{ /*do not close*/ /*we pattern are singletone source*/}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate) = 0;
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource) = 0;
	  virtual void startStream(unsigned clientSessionId, void* streamToken,
				   TaskFunc* rtcpRRHandler,
				   void* rtcpRRHandlerClientData,
				   unsigned short& rtpSeqNum,
	                           unsigned& rtpTimestamp,
				   ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
	                           void* serverRequestAlternativeByteHandlerClientData)
	  {
		  OnDemandServerMediaSubsession::startStream(clientSessionId,
				  streamToken,
				  rtcpRRHandler,
				  rtcpRRHandlerClientData,
				  rtpSeqNum,
				  rtpTimestamp,
				  serverRequestAlternativeByteHandler,
				  serverRequestAlternativeByteHandlerClientData);
	  }
};
/*--------------------------------------------------------------------------------
 server media subsession for h264 frame
 --------------------------------------------------------------------------------*/
class livemediapp_h264_servermediasubsession : public livemediapp_servermediasubession
{
public:
	livemediapp_h264_servermediasubsession(FramedSource *filteredsource) :
		livemediapp_servermediasubession(filteredsource->envir(),
						filteredsource){}
	virtual ~livemediapp_h264_servermediasubsession(){}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
						      unsigned& estBitrate)
	{
		estBitrate = 500;
		if(!_source)
		{
			/*
			 	 for the syncronized framed
			 */
			_source = H264VideoStreamFramer::createNew(envir(), _filtersource);
		}
		return _source;
	}
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
					unsigned char rtpPayloadTypeIfDynamic,
					FramedSource* inputSource)
	{
		 return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
	}
};
/*--------------------------------------------------------------------------------
 server media subsession for aac (adts)format
 --------------------------------------------------------------------------------*/
class livemediapp_adts_audio_servermediasubsession : public livemediapp_servermediasubession
{
	int profile;
	int frequency;
	int channel;
public:
	livemediapp_adts_audio_servermediasubsession(mediacontainer &container,/*saving container informations*/
			FramedSource *filteredsource) :
				livemediapp_servermediasubession(filteredsource->envir(),
						filteredsource),
	 profile(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->profile),
	 frequency(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate),
	 channel(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->channels) { 	}
	virtual ~livemediapp_adts_audio_servermediasubsession(){}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
						      unsigned& estBitrate)
	{
		if(!_source)
		{
			_source = _filtersource;
		}
		return _source;
	}
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
					unsigned char rtpPayloadTypeIfDynamic,
					FramedSource* inputSource)
	{
		  // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
		int frequencyindex = 0;
		if(96000 == frequency) frequencyindex = 0;
		if(88200 == frequency) frequencyindex = 1;
		if(64000 == frequency) frequencyindex = 2;
		if(48000 == frequency) frequencyindex = 3;
		if(44100 == frequency) frequencyindex = 4;
		if(32000 == frequency) frequencyindex = 5;
		if(24000 == frequency) frequencyindex = 6;
		if(22050 == frequency) frequencyindex = 7;
		if(16000 == frequency) frequencyindex = 8;
		if(12000 == frequency) frequencyindex = 9;
		if(11025 == frequency) frequencyindex = 10;
		if(8000 == frequency) frequencyindex = 11;
		if(7350 == frequency) frequencyindex = 12;
		if(0 == frequency) frequencyindex = 13;
		if(0 == frequency) frequencyindex = 14;
		if(0 == frequency) frequencyindex = 15;

		 unsigned char audioSpecificConfig[2];
		 char fConfigStr[5];
		 u_int8_t const audioObjectType = profile + 1;
		 audioSpecificConfig[0] = (audioObjectType<<3) | (frequencyindex>>1);
		 audioSpecificConfig[1] = (frequencyindex<<7) | (channel<<3);
		 sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

		return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, frequency, "audio", "AAC-hbr", fConfigStr, channel);
	}
};

/*--------------------------------------------------------------------------------
 our frame sources
 --------------------------------------------------------------------------------*/
	class mediafiltered_source : public FramedSource
	{
	/*--------------------------------------------------------------------------------
	 filter source interface
	 --------------------------------------------------------------------------------*/
		livemediapp_serversession &_ourserversession;/*just reference*/
		enum AVMediaType _mediaid;
		TaskToken _misstoken;/*we reading retry using 'timer'*/
	public:
		mediafiltered_source(
				livemediapp_serversession &ourserversession,
				enum AVMediaType mediaid) :
				FramedSource(ourserversession.envir()),
				_ourserversession(ourserversession),
				_mediaid(mediaid),
				_misstoken(nullptr) { }
		virtual ~mediafiltered_source()
		{
				stop_misstoken();
		}
		void stop_misstoken()
		{
			if(_misstoken)
			{
				envir().taskScheduler().unscheduleDelayedTask(_misstoken);
				_misstoken = nullptr;
			}
		}
		void retry(unsigned nextime/*us*/)
		{
			stop_misstoken();
			_misstoken = envir().taskScheduler().scheduleDelayedTask(nextime,
					[](void* c)->void{((mediafiltered_source *)c)->doGetNextFrame();},
					this);
		}
		virtual void doGetNextFrame()
		{
			do
			{
				if(!isCurrentlyAwaitingData())
				{
					/*if our server session scheduler so fast*/
					break;
				}

				fFrameSize = _ourserversession.doGetNextFrame(_mediaid,
						fTo,
						fMaxSize,
						&fNumTruncatedBytes,
						&fPresentationTime,
						&fDurationInMicroseconds);
				if(fFrameSize <= 0)
				{
					/*if our server session buffer too slow*/
					break;
				}

				/*
					now our source can be deliver to live555 scheduler
				*/
				FramedSource::afterGetting(this);
				return;
			}while(0);
			/*
			 	 do retry if can't reading from source
			 */
			retry(10);
		}
		virtual void doStopGettingFrames()
		{
			stop_misstoken();
			_ourserversession.doStopGettingFrames(_mediaid);
		}
	};
	/*--------------------------------------------------------------------------------
	 base source
	 --------------------------------------------------------------------------------*/
	class source
	{
		friend class livemediapp_serversession;
	protected:
		unsigned char *_bank[AVMEDIA_TYPE_NB];/*buffering*/
		unsigned _index[AVMEDIA_TYPE_NB];
		unsigned _max[AVMEDIA_TYPE_NB];
		void bank_realloc(enum AVMediaType type, unsigned size)
		{
			/*
			 	 our bank allocation
			 */
			if(size <= 0) size = 100;
			if(_max[type] <= 0)
			{
				_bank[type] = (unsigned char *)__base__malloc__(size);
				_index[type] = 0;
				_max[type] = size;
				return;
			}
			if(_max[type] < size)
			{
				unsigned char *temp = (unsigned char *)__base__malloc__(size);
				memcpy(temp, _bank[type], _index[type]);
				__base__free__(_bank[type]);
				_bank[type] = temp;
				_max[type] = size;
			}
		}
		void bank_in(enum AVMediaType type, unsigned char *from, unsigned size)
		{
			/*
			 	 data input
			 */
			source::bank_realloc(type, _index[type] + size);
			memcpy(_bank[type] + _index[type],from,  size);
			_index[type] += size;
		}
		int bank_drain(enum AVMediaType type, unsigned char *to, unsigned size)
		{
			size = _index[type] > size ? size : _index[type];
			if(size > 0)
			{
				memcpy(to, _bank[type], size);
				_index[type] -= size;
				if(_index[type] > 0)
				{
					memcpy(_bank[type], _bank[type] + size, _index[type]);
				}
				return size;
			}
			return 0;
		}
		source(){for(int i = 0; i < AVMEDIA_TYPE_NB; i++){_bank[i] = nullptr;_max[i] = 0; _index[i] = 0;}}
		virtual ~source()
		{
			for(int i = 0; i < AVMEDIA_TYPE_NB; i++) { if(_bank[i]) __base__free__(_bank[i]); }
		}
		virtual int doGetNextFrame(enum AVMediaType type,
				unsigned char*to,
				unsigned size,
				unsigned *numtruncatedbyte,
				struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			*numtruncatedbyte = 0;
			/*
			 	 default flushing
			 */
			bank_realloc(type, size);

			return bank_drain(type, to, _index[type]);
		}
		virtual void doStopGettingFrames(enum AVMediaType type) = 0;

	};

	class uvcsource : public source
	{
		friend class livemediapp_serversession;

		/*--------------------------------------------------------------------------------
		 uvc source
		 --------------------------------------------------------------------------------*/
		uvc *_uvc;
		encoder *_enc;
		avattr _attr;
		virtual ~uvcsource()
		{
			if(_enc) delete _enc;
			if(_uvc) delete _uvc;
		}
		uvcsource(livemediapp_serversession &serversession,
				const char *src) : source(),
				_uvc(nullptr),
				_enc(nullptr),
				_attr(avattr())
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
				_attr.set(avattr_key::pixel_format, avattr_key::pixel_format, (int)AV_PIX_FMT_YUV420P, 0.0);
				_enc = new encoder(_attr);


				serversession.addSubsession(new livemediapp_h264_servermediasubsession(new mediafiltered_source(serversession, AVMEDIA_TYPE_VIDEO)));
		}
		virtual int doGetNextFrame(enum AVMediaType type,
				unsigned char*to,
				unsigned size,
				unsigned *numtruncatedbyte,
				struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			*numtruncatedbyte = 0;
			int readsize = 0;

			/*
			 	 first. flush data if bank has data
			 */
			readsize = source::doGetNextFrame(type,
					to,
					size,
					numtruncatedbyte,
					presentationtime,
					durationinmicroseconds);
			if(readsize <= 0)/*no flush data*/
			{
				int res = _uvc->waitframe(0);/*tell to uvc 'has frame'*/
				DECLARE_THROW(res < 0, "source uvc has broken");
				if(res > 0)/* res == 0  noframe*/
				{
					_uvc->get_videoframe([&](pixelframe &pix)->void {
							{
								/*
								 	 convert request format
								 */
								swxcontext_class (pix, _attr);
							}
							_enc->encoding(pix,[&](avframe_class &frm,
									avpacket_class &pkt,
									void * )->void{
								source::bank_in(type, pkt.raw()->data, pkt.raw()->size);
								readsize = source::bank_drain(type, to, size);
							});
						});
				}
			}
			if(readsize > 0)
			{
				gettimeofday(presentationtime, nullptr);
			}
			return readsize;
		}
		virtual void doStopGettingFrames(enum AVMediaType type)
		{
			/*no condition.. just wait */
		}
	};
	class mp4mediacontainersource : public source
	{
		friend class livemediapp_serversession;
		mediacontainer *_container;
		avpacket_bsf *_mp4_h264_adts_bsf;
		uint64_t _per_frame;
		void reupload()
		{
			std::string target = _container->filename();
			delete _mp4_h264_adts_bsf;
			delete _container;

			_container = new mediacontainer(target.c_str());
			_mp4_h264_adts_bsf = new avpacket_bsf(_container);
		}
		void audio_presentationtime(struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			/*
			 	 our filtered source can't know audio bps.
			 	 so we caluation next presentaion bps.
			 */
			  if (presentationtime->tv_sec == 0 && presentationtime->tv_usec == 0) {
			    // This is the first frame, so use the current time:
			    gettimeofday(presentationtime, NULL);
			  } else {
			    // Increment by the play time of the previous frame:
			    unsigned uSeconds = presentationtime->tv_usec + _per_frame;
			    presentationtime->tv_sec += uSeconds/1000000;
			    presentationtime->tv_usec = uSeconds%1000000;
			  }

			  *durationinmicroseconds = _per_frame;
		}
		mp4mediacontainersource(livemediapp_serversession &serversession,
				const char *src) : source(),
						_container(nullptr),
						_mp4_h264_adts_bsf(nullptr),
						_per_frame(0)

		{
			OutPacketBuffer::increaseMaxSizeTo(1000000);

			_container = new mediacontainer(src);
			_mp4_h264_adts_bsf = new avpacket_bsf(_container);

			if(_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_VIDEO))
			{
				serversession.addSubsession(new livemediapp_h264_servermediasubsession(new mediafiltered_source(serversession, AVMEDIA_TYPE_VIDEO)));
			}
			if(_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO))
			{
				/*
				 	      codec->frame_size  : The following data should not be initialized.

							  Number of samples per channel in an audio frame.
				 */
				 _per_frame = (_container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->frame_size * 1000000) /
						 _container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate;

				serversession.addSubsession(new livemediapp_adts_audio_servermediasubsession(*_container,/*get audio spec information*/
						new mediafiltered_source(serversession, AVMEDIA_TYPE_AUDIO)));
			}
		}
		virtual ~mp4mediacontainersource()
		{
			if(_mp4_h264_adts_bsf) delete _mp4_h264_adts_bsf;
			if(_container) delete _container;
		}
		virtual int doGetNextFrame(enum AVMediaType type,
				unsigned char*to,
				unsigned size,
				unsigned * numtruncatedbyte,
				struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			int readsize = 0;

			*numtruncatedbyte = 0;
			readsize = source::doGetNextFrame(type,
					to,
					size,
					numtruncatedbyte,
					presentationtime,
					durationinmicroseconds);
			if(readsize > 0)
			{
				return readsize;
			}

			avpacket_class  pkt;
			bool res = _container->read_packet(type, pkt);
			if(!res)
			{
				if(_container->eof_stream())
				{
					reupload();
					return doGetNextFrame(type,
							to,
							size,
							numtruncatedbyte,
							presentationtime,
							durationinmicroseconds);

				}
				else
				{
					/*wait until drain another media*/
					return 0;
				}
			}

			/*
			 	 filtering to raw encoded data
			 */

			(*_mp4_h264_adts_bsf)(pkt);

			source::bank_in(type, pkt.raw()->data, pkt.raw()->size);
			readsize = source::bank_drain(type, to, size);
			if(type  == AVMEDIA_TYPE_AUDIO &&
					readsize > 0)
			{
				audio_presentationtime(presentationtime, durationinmicroseconds);
			}
			return readsize;
		}
		virtual void doStopGettingFrames(enum AVMediaType type)
		{
			/*no condition.. just wait */
		}

	};


	int doGetNextFrame(enum AVMediaType type,
			unsigned char*to,
			unsigned size,
			unsigned *numtruncatedbyte,
			struct timeval *presentationtime,
			unsigned *durationinmicroseconds)
	{
		return _our_static_source->doGetNextFrame(type,
				to,
				size,
				numtruncatedbyte,
				presentationtime,
				durationinmicroseconds);
	}


	virtual void doStopGettingFrames(enum AVMediaType type)
	{
		return _our_static_source->doStopGettingFrames(type);
	}
	source *_our_static_source;
	source *createnew_relative(char const *srcs, livemediapp_serversession &serversession)
	{
		if(contain_string(srcs, "/dev")) return new uvcsource(serversession, srcs);
		if(contain_string(srcs, ".mp4")) return new mp4mediacontainersource(serversession, srcs);
		return nullptr;
	}

	bool create_subsessions(char const *srcs)
	{
		_our_static_source = createnew_relative(srcs, *this);
		if(!_our_static_source)
		{
			return false;
		}
		return numSubsessions() > 0;
	}
public:
	static ServerMediaSession *createnew(char const *srcs,
				UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines)
	{
		livemediapp_serversession *newsession = new livemediapp_serversession(env, streamName, info, description, isSSM, miscSDPLines);
		DECLARE_THROW(!newsession->create_subsessions(srcs), "no found stream");
		return newsession;
	}
	livemediapp_serversession(UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines) :
		    	   ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines),
				   _our_static_source(nullptr){ }
	virtual ~livemediapp_serversession()
	{
		/*when subsession delete, _our_static_source deleted */
	}

};


