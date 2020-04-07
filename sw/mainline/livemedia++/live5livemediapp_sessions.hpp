#pragma once

/*--------------------------------------------------------------------------------
 our server session
 --------------------------------------------------------------------------------*/
class livemediapp_serversession : public ServerMediaSession
{
	public:typedef std::function<void (unsigned clientsessionid,
			enum AVMediaType mediatype,
			enum AVCodecID codecid,
			unsigned char *streamdata,
			unsigned streamdata_size)> stream_out_ref;
private:
	/*--------------------------------------------------------------------------------
	 	 our reference stream out filter
	 --------------------------------------------------------------------------------*/

	struct streamoutfilter_par
	{
		unsigned _clientsessionid;
		enum AVMediaType _mediatype;
		enum AVCodecID _codecid;
		unsigned char *_streamdata;
		unsigned _streamdata_size;
		streamoutfilter_par(unsigned clientsessionid,
		enum AVMediaType mediatype,
		enum AVCodecID codecid,
		unsigned char *_streamdata,
		unsigned streamdata_size) :
			_clientsessionid(clientsessionid),
			_mediatype(mediatype),
			_codecid(codecid),
			_streamdata(_streamdata),
			_streamdata_size(streamdata_size){}
	};
	class streamoutfilter :
			public filter<streamoutfilter_par,
			livemediapp_serversession>
	{
		friend class livemediapp_serversession;
		stream_out_ref soref;
		streamoutfilter(livemediapp_serversession *p) :
			filter<streamoutfilter_par,
			livemediapp_serversession>(p),
			soref(nullptr){}

		virtual void operator >>(streamoutfilter_par &&pf)
		{
			if(soref)
			{
				soref(pf._clientsessionid,
						pf._mediatype,
						pf._codecid,
						pf._streamdata,
						pf._streamdata_size);
			}
		}
		void register_soref(const stream_out_ref &fn)
		{
			soref = fn;
		}
	};

	/*--------------------------------------------------------------------------------
	 our frame sources
	 --------------------------------------------------------------------------------*/

		/*--------------------------------------------------------------------------------
		 base source
		 --------------------------------------------------------------------------------*/
		class source
		{
			friend class livemediapp_serversession;
		protected:
			unsigned _clientsessionid;/*registed client session id*/
			unsigned _refcount;/*subsession reference count*/
			unsigned char *_bank[AVMEDIA_TYPE_NB];/*buffering*/
			unsigned _index[AVMEDIA_TYPE_NB];
			unsigned _max[AVMEDIA_TYPE_NB];

			unsigned clientid()
			{
				return _clientsessionid;
			}
			unsigned reference()
			{
				return _refcount;
			}
			void reference_increase()
			{
				_refcount++;
			}
			void reference_decrease()
			{
				if(_refcount > 0)
				{
					_refcount--;
				}
			}
			
			void bank_realloc(enum AVMediaType type, unsigned size)
			{
				/*
					 our bank allocation
				 */
				if(size <= 0) size = 100;
				if(_max[type] <= 0)
				{
					printf("client %d bank(%s) realloc %d -> %d\n",_clientsessionid,
						type == AVMEDIA_TYPE_VIDEO ? "video" : "audio", 
						_max[type],
						size);
					_bank[type] = (unsigned char *)__base__malloc__(size);
					_index[type] = 0;
					_max[type] = size;
					return;
				}
				if(_max[type] < size)
				{
					printf("client %d bank(%s) realloc %d -> %d\n", _clientsessionid,
						type == AVMEDIA_TYPE_VIDEO ? "video" : "audio", 
						_max[type],
						size);
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
				return size;
			}
			source(unsigned clientsessionid) :
				_clientsessionid(clientsessionid),
					_refcount(0)
				{for(int i = 0; i < AVMEDIA_TYPE_NB; i++){_bank[i] = nullptr;_max[i] = 0; _index[i] = 0;}}
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
	
				return bank_drain(type, to, size);
			}
			virtual void seekFrame(char*& absStart, char*& absEnd) = 0;
			virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes) = 0;
			virtual enum AVCodecID videocodec() = 0;
			virtual enum AVCodecID audiocodec() = 0;
			virtual void doStopGettingFrames(enum AVMediaType type) = 0;
	
		};
		/*--------------------------------------------------------------------------------
		 uvc source
		 --------------------------------------------------------------------------------*/
#if defined have_uvc
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
			uvcsource(const char *src,
					unsigned clientid) : source(clientid),
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
					if(res > 0)/* res == 0	noframe*/
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
			virtual void seekFrame(char*& absStart, char*& absEnd)
			{
				/*can't seek uvc*/
			}
			virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
			{
				/*can't seek uvc*/
			}
	
			virtual enum AVCodecID videocodec()
			{
				if(_enc && _uvc)
				{
					return (enum AVCodecID)_attr.get_int(avattr_key::video_encoderid);
				}
				return AV_CODEC_ID_NONE;
			}
			virtual enum AVCodecID audiocodec()
			{
				/*uvc current has no audio*/
				return AV_CODEC_ID_NONE;
			}
		};

#endif

		class mp4mediacontainersource : public source
		{
		friend class livemediapp_serversession;
		/*--------------------------------------------------------------------------------
			mediacontainer (mp4) source
		 --------------------------------------------------------------------------------*/
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
			mp4mediacontainersource(const char *src,
					unsigned clientid) : source(clientid),
							_container(nullptr),
							_mp4_h264_adts_bsf(nullptr),
							_per_frame(0)
	
			{
				OutPacketBuffer::increaseMaxSizeTo(LIVE5_BUFFER_SIZE);
	
				_container = new mediacontainer(src);
				_mp4_h264_adts_bsf = new avpacket_bsf(_container);
	
				if(_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO))
				{
					/*
							  codec->frame_size  : The following data should not be initialized.
	
								  Number of samples per channel in an audio frame.
					 */
					 _per_frame = (_container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->frame_size * 1000000) /
							 _container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate;
				}
			}
			virtual ~mp4mediacontainersource()
			{
			printf("deleted 11\n");
				if(_mp4_h264_adts_bsf) delete _mp4_h264_adts_bsf;
				printf("deleted22\n");
				if(_container) delete _container;
				printf("deleted33\n");
	
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
	
				avpacket_class	pkt;
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
			virtual void seekFrame(char*& absStart, char*& absEnd)
			{
			
			}
			virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
			{
			
			}
			virtual enum AVCodecID videocodec()
			{
				return _mp4_h264_adts_bsf &&
						_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_VIDEO) &&
						AV_CODEC_ID_H264 == _container->find_codec(AVMEDIA_TYPE_VIDEO) ? 
						AV_CODEC_ID_H264 : AV_CODEC_ID_NONE;
	
			}
			virtual enum AVCodecID audiocodec()
			{
				return _mp4_h264_adts_bsf &&
						_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO) &&
						AV_CODEC_ID_AAC == _container->find_codec(AVMEDIA_TYPE_AUDIO) ? 
						AV_CODEC_ID_AAC : AV_CODEC_ID_NONE;
	
			}
	
		};
	
		

		class mediafiltered_source : public FramedSource
		{
		/*--------------------------------------------------------------------------------
		 filter source interface : source and serversession interface
		 --------------------------------------------------------------------------------*/
			livemediapp_serversession &_ourserversession;/*just reference*/
			enum AVMediaType _mediaid;
			TaskToken _misstoken;/*we reading retry using 'timer'*/
			unsigned _clientid;
		public:
			mediafiltered_source(
					livemediapp_serversession &ourserversession,
					enum AVMediaType mediaid,
					unsigned clientid) :
					FramedSource(ourserversession.envir()),
					_ourserversession(ourserversession),
					_mediaid(mediaid),
					_misstoken(nullptr),
					_clientid(clientid){ }
			virtual ~mediafiltered_source()
			{
					stop_misstoken();
			}
			unsigned clientid()
			{
				return _clientid;
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
					fFrameSize = _ourserversession.doGetNextFrame(_clientid,
						_mediaid,
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
				retry(0);
			}
			virtual void doStopGettingFrames()
			{
				stop_misstoken();
				_ourserversession.doStopGettingFrames(_clientid,
					_mediaid);
			}
			void seekFrame(char*& absStart, char*& absEnd)
			{
				_ourserversession.seekFrame(_clientid,
					absStart,
					absEnd);
			}
			void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
			{
				_ourserversession.seekFrame(_clientid,
					seekNPT,
					streamDuration, 
					numBytes);
			}
				
		};
		

	/*--------------------------------------------------------------------------------
	 our server media subsessions
	 --------------------------------------------------------------------------------*/

class livemediapp_servermediasubession :
		public OnDemandServerMediaSubsession
{
		/*based subsession*/
protected:
	livemediapp_serversession &_serversession;
	enum AVMediaType _type;/*for debug print*/
	

	livemediapp_servermediasubession(livemediapp_serversession &serversession,
		enum AVMediaType type,
		Boolean bresuse) :
		OnDemandServerMediaSubsession(serversession.envir(), bresuse),
			_serversession(serversession),
			_type(type) {}
	virtual ~livemediapp_servermediasubession()
	{

	}
	virtual void closeStreamSource(FramedSource *inputSource) = 0;	
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

			printf("client %d start subsession stream : %s..\n",clientSessionId,
				_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio");
		  OnDemandServerMediaSubsession::startStream(clientSessionId,
				  streamToken,
				  rtcpRRHandler,
				  rtcpRRHandlerClientData,
				  rtpSeqNum,
				  rtpTimestamp,
				  serverRequestAlternativeByteHandler,
				  serverRequestAlternativeByteHandlerClientData);
	  }
	virtual void pauseStream(unsigned clientSessionId, void *streamToken)
  	{
  		/*
	  		we have nothing to do. because the sink has no request source's data.	  		
	  	*/
			printf("client %d pause subsession stream : %s..\n", clientSessionId,
			_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio");

		
  		OnDemandServerMediaSubsession::pauseStream(clientSessionId, streamToken);
  	}
	virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes)
	{


			/*
				OnDemandServerMediaSubsession has no imple
			*/
		OnDemandServerMediaSubsession ::seekStreamSource (inputSource, seekNPT, streamDuration, numBytes);
		mediafiltered_source *_filtersource = dynamic_cast<mediafiltered_source *>(inputSource);
		if(_filtersource)/*reference for client id*/
		{
			_filtersource->seekFrame(seekNPT, streamDuration, numBytes);
			printf("client %d seek subsession stream : %s.. (seekNPT = %f / streamDuration = %f, numByte = %d)\n",_filtersource->clientid(),
				_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio",			

			seekNPT, 
			streamDuration,
			numBytes);
		}

	}

  virtual void seekStreamSource(FramedSource* inputSource, char*& absStart, char*& absEnd)
	{

			/*
				OnDemandServerMediaSubsession has no imple
			*/
		OnDemandServerMediaSubsession::seekStreamSource(inputSource, absStart, absEnd);
		mediafiltered_source *_filtersource = dynamic_cast<mediafiltered_source *>(inputSource);
		if(_filtersource)/*reference for client id*/
		{
			_filtersource->seekFrame(absStart, absEnd);
			printf("client %d seek subsession stream : %s.. (absstart = %s / absend = %s)\n", _filtersource->clientid(),
				_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio",
			absStart ? absStart : "unknown",
			absEnd ? absEnd : "unknown");
		}
	}

  virtual void setStreamSourceScale(FramedSource* inputSource, float scale)
	{
	OnDemandServerMediaSubsession::setStreamSourceScale(inputSource, scale);
	}
  virtual void setStreamSourceDuration(FramedSource* inputSource, double streamDuration, u_int64_t& numBytes)
	{
		OnDemandServerMediaSubsession::setStreamSourceDuration(inputSource, streamDuration, numBytes);
	}
};
/*--------------------------------------------------------------------------------
 server media subsession for h264 frame
 --------------------------------------------------------------------------------*/
class livemediapp_h264_servermediasubsession : public livemediapp_servermediasubession
{
public:
	livemediapp_h264_servermediasubsession(livemediapp_serversession &serversession, Boolean breuse) :
		livemediapp_servermediasubession(serversession, AVMEDIA_TYPE_VIDEO, breuse){}
	virtual ~livemediapp_h264_servermediasubsession(){}
	virtual void closeStreamSource(FramedSource *inputSource)
	{  

		H264VideoStreamFramer *source = (H264VideoStreamFramer *)inputSource;
		mediafiltered_source *filter = (mediafiltered_source*)source->inputSource();
		unsigned clientsessionid = filter->clientid();
		/*
			close source for delete serversession's source dependency
		*/
		Medium::close(source);


		if(clientsessionid) _serversession.closeStreamSource(clientsessionid);
	}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
						      unsigned& estBitrate)
	{
	
	/*
		ondemand create dummy sink for sdp using 'clinetSessionId = 0'
		so we do not create last source for speed with memory 
	*/
		if(clientSessionId) _serversession.createNewStreamSource(clientSessionId);
		return H264VideoStreamFramer::createNew(envir(),new mediafiltered_source(_serversession, 
			AVMEDIA_TYPE_VIDEO, 
			clientSessionId));
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
	livemediapp_adts_audio_servermediasubsession(mediacontainer &container, livemediapp_serversession &serversession, Boolean breuse) :
				livemediapp_servermediasubession(serversession, AVMEDIA_TYPE_AUDIO, breuse),
	 profile(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->profile),
	 frequency(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate),
	 channel(container.find_stream(AVMEDIA_TYPE_AUDIO)->codec->channels) { 	}
	virtual ~livemediapp_adts_audio_servermediasubsession(){}
	virtual void closeStreamSource(FramedSource *inputSource)
	{  
		unsigned clientid = 0;
		mediafiltered_source *filter = (mediafiltered_source*)inputSource;
		clientid = filter->clientid();
		/*
			close source for delete serversession's source dependency
		*/
		Medium::close(filter);

		if(clientid)_serversession.closeStreamSource(clientid);
	}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
						      unsigned& estBitrate)
	{
		/*
		ondemand create dummy sink for sdp using 'clinetSessionId = 0'
		so we do not create last source for speed with memory 
	*/
	
	 if(clientSessionId) _serversession.createNewStreamSource(clientSessionId);
		return new mediafiltered_source(_serversession, 
			AVMEDIA_TYPE_AUDIO, 
			clientSessionId);
	}
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
					unsigned char rtpPayloadTypeIfDynamic,
					FramedSource* inputSource)
	{
		  // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
		int frequencyindex = 0;
		if(96000 == frequency) frequencyindex = 0;
		else if(88200 == frequency) frequencyindex = 1;
		else if(64000 == frequency) frequencyindex = 2;
		else if(48000 == frequency) frequencyindex = 3;
		else if(44100 == frequency) frequencyindex = 4;
		else if(32000 == frequency) frequencyindex = 5;
		else if(24000 == frequency) frequencyindex = 6;
		else if(22050 == frequency) frequencyindex = 7;
		else if(16000 == frequency) frequencyindex = 8;
		else if(12000 == frequency) frequencyindex = 9;
		else if(11025 == frequency) frequencyindex = 10;
		else if(8000 == frequency) frequencyindex = 11;
		else if(7350 == frequency) frequencyindex = 12;
		else if(0 == frequency) frequencyindex = 13;
		else if(0 == frequency) frequencyindex = 14;
		else if(0 == frequency) frequencyindex = 15;

		 unsigned char audioSpecificConfig[2];
		 char fConfigStr[5];
		 u_int8_t const audioObjectType = profile + 1;
		 audioSpecificConfig[0] = (audioObjectType<<3) | (frequencyindex>>1);
		 audioSpecificConfig[1] = (frequencyindex<<7) | (channel<<3);
		 sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

		return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, frequency, "audio", "AAC-hbr", fConfigStr, channel);
	}
};




	virtual void closeStreamSource(unsigned clientSessionId)
	{  
		/*calling subsession*/
		source *fsource = nullptr;
		
		std::list<source *>::iterator it;

		it = _sources.begin();
		while(it != _sources.end())
		{
			if((*it)->clientid() == clientSessionId)
			{
				(*it)->reference_decrease();
				if((*it)->reference() <= 0)
				{
					printf("delete source : %d\n", (*it)->clientid());

					source *delsource = *it;
					_sources.erase(it);/*broken 'it' but next just break.*/
					delete delsource;
					
				}
				break;
			}
			else ++it;
		}

	}
	
	virtual  void createNewStreamSource(unsigned clientSessionId)
	{
	/*calling subsession*/
		source *fsource = nullptr;
		unsigned reference_max = numSubsessions();
		/*
			duplicated check
		*/
		for(auto &it : _sources)
		{
			if(it->clientid() == clientSessionId)
			{
				fsource = it;
				break;
			}
		}
		/*
			create source if first
		*/
		if(!fsource)
		{
		
			if(contain_string(_srcs, "/dev"))
			{
				uvcsource *oursource = new uvcsource(_srcs,  clientSessionId);
				fsource = (source *)oursource;
			}
			if(contain_string(_srcs, ".mp4"))
			{
				mp4mediacontainersource *oursource = new mp4mediacontainersource(_srcs,  clientSessionId);
				fsource = (source *)oursource;
			}
			/*
				_source pointer has always true , 
				because we check when 'create_subsessions()' function called 
			*/
			fsource->reference_increase();
			_sources.push_back(fsource);
			printf("source was create %s ref(%d) clientsessionid(%d)\n", _srcs, 
				fsource->reference(), 
				fsource->clientid());
		}
		/*
			reference add if exist source
		*/
		else
		{
			if(fsource->reference() > reference_max)
			{
				/*invalid request. not occur*/
				return;
			}
			fsource->reference_increase();
			
			printf("source was referenced %s ref(%d) clientsessionid(%d)\n", _srcs, 
			fsource->reference(), 
			fsource->clientid());
		}

	}

	int doGetNextFrame(unsigned clientsessionid, 
		enum AVMediaType type,
		unsigned char*to,
		unsigned size,
		unsigned *numtruncatedbyte,
		struct timeval *presentationtime,
		unsigned *durationinmicroseconds)
	{
	/*calling filtered source*/
		int readsize = 0;
		find_and(clientsessionid, 
			[&](source *t)->int{

			readsize =  t->doGetNextFrame(type,
				to,
				size,
				numtruncatedbyte,
				presentationtime,
				durationinmicroseconds);

			_sofilter << (streamoutfilter_par(clientsessionid,
					type,
					type == AVMEDIA_TYPE_AUDIO ? t->audiocodec() : t->videocodec(),
					to,
					readsize));
		});
		return readsize;
	}
	virtual void doStopGettingFrames(unsigned clientsessionid, 
		enum AVMediaType type)
	{
	/*calling filtered source*/
		find_and(clientsessionid, 
			[&](source *t)->void{
			t->doStopGettingFrames(type);
		});
	}
	void seekFrame(unsigned clientsessionid, 
		char*& absStart, 
		char*& absEnd)
	{
	/*calling filtered source*/
		find_and(clientsessionid, 
			[&](source *t)->void{
			t->seekFrame(absStart, absEnd);
		});
		
	}
	void seekFrame(unsigned clientsessionid, 
		double& seekNPT, 
		double streamDuration, 
		u_int64_t& numBytes)
	{
	/*calling filtered source*/
		find_and(clientsessionid, 
			[&](source *t)->void{
			t->seekFrame(seekNPT, streamDuration, numBytes);
		});		
	}
	template <typename functor>
	void find_and(unsigned clientsessionid, 
		functor && fn)
	{
		for(auto &it : _sources)
		{
			if(it->clientid() == clientsessionid)
			{
				fn(it);
				break;
			}
		}
	}
	bool create_subsessions()
	{
		/*tell subsession*/
		if(contain_string(_srcs, "/dev"))
		{
			uvcsource dump( _srcs, 0);
			if(dump.videocodec() == AV_CODEC_ID_H264)
			{
				this->addSubsession(new livemediapp_h264_servermediasubsession(*this, false/*uvc has static instance*/));
			}
		}
		if(contain_string(_srcs, ".mp4"))
		{
			mp4mediacontainersource dump(_srcs, 0);
			/*
				selete reuse 
				if reuse == false
				clientid not important value. live555 create source just 1.
				therefor we operation just 1 clientsession id 
			*/
			Boolean breuse = false;
			if(dump.videocodec() == AV_CODEC_ID_H264)
			{
				this->addSubsession(new livemediapp_h264_servermediasubsession(*this, breuse));
			}
			if(dump.audiocodec() == AV_CODEC_ID_AAC)
			{
				this->addSubsession(new livemediapp_adts_audio_servermediasubsession(*dump._container,/*get parameter audio spec*/
					*this,
					breuse));
			}
		}
		return numSubsessions() > 0;
	}
	std::list<source *>_sources;;
	char *_srcs;


	streamoutfilter _sofilter;
public:
	static ServerMediaSession *createnew(char const *srcs,
				UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines)
	{
		livemediapp_serversession *newsession = new livemediapp_serversession(srcs, 
			env, 
			streamName, 
			info, 
			description, 
			isSSM, 
			miscSDPLines);
		
		DECLARE_THROW(!newsession->create_subsessions(), "no found stream");
		return newsession;
	}
	livemediapp_serversession(char const *srcs, 
		UsageEnvironment& env,
		       char const* streamName,
		       char const* info,
		       char const* description,
		       Boolean isSSM,
		       char const* miscSDPLines) :
		    	   ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines),
					_srcs(strdup(srcs)),
					_sofilter(this){ }
	virtual ~livemediapp_serversession()
	{
		free(_srcs);
	}
	void start_streamout_filter(const stream_out_ref &f)
	{
		_sofilter.register_soref(f);
		_sofilter.enable();
	}
	void end_streamout_filter()
	{
		_sofilter.disable();
	}

};


