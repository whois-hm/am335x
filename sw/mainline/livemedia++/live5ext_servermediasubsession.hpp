#pragma once


class live5ext_servermdiasubsession : public OnDemandServerMediaSubsession
{
private:
	/*
		stored live5extsource_uvc's parameters
	*/
	char const *_dev;
	UsageEnvironment &env;
	live5extsource_uvc *_our_dev_source;
	FramedSource *_our_single_source;

	live5ext_servermdiasubsession(char const *device,
		UsageEnvironment &env) : 
		OnDemandServerMediaSubsession(env, True/*reuse source*/),
		_dev(device),
		env(env),
		_our_dev_source(nullptr),
		_our_single_source(nullptr){}

public:
	static live5ext_servermdiasubsession *
		createnew(char const *device,
		UsageEnvironment &env)
	{
		return new live5ext_servermdiasubsession(device, env);
	}
	virtual ~live5ext_servermdiasubsession()
	{
		if(_our_single_source)
		{
			Medium::close(_our_single_source);
		}
		if(_our_dev_source)
		{
			Medium::close(_our_dev_source);
		}
	}
	virtual void closeStreamSource(FramedSource *inputSource)
	{
		/*maintain signleton source*/
		//return;
		//Medium::close(inputSource);
	}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate)
	{
			//return H264VideoStreamFramer::createNew(env, new live5extsource_uvc(_dev, env));
	printf("request create new stream source\n");
		if(!_our_single_source )
		{

			_our_dev_source = new live5extsource_uvc(_dev, env);
			printf("create new stream source %08x\n", _our_dev_source);
			if(!_our_dev_source->has_video() ||
					_our_dev_source->video_codec() != AV_CODEC_ID_H264)
			{
				delete _our_dev_source;
				_our_dev_source = nullptr;
			}
			if(_our_dev_source)
			{
				_our_single_source = (FramedSource*) H264VideoStreamFramer::createNew(env, _our_dev_source);
				printf("create new stream source framer %08x\n", _our_single_source);
			} 
		}


		return _our_single_source; 
	}

	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource)
	{
		live5extsource_uvc *source = dynamic_cast<live5extsource_uvc *>(inputSource);
		/*
			support parameter check in " createNewStreamSource "fucntion
		*/

		return H264VideoRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic);
		
	}
};

