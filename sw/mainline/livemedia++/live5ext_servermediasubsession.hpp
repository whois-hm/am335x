#pragma once


class live5ext_servermdiasubsession : public OnDemandServerMediaSubsession
{
private:
	/*
		stored live5extsource_uvc's parameters
	*/
	char const *_dev;
	UsageEnvironment &env;
	FramedSource *_our_single_source;

	live5ext_servermdiasubsession(char const *device,
		UsageEnvironment &env) : 
		OnDemandServerMediaSubsession(env, True/*reuse source*/),
		_dev(device),
		env(env),
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
	}
	virtual void closeStreamSource(FramedSource *inputSource)
	{
		/*maintain signleton source*/
		return;
	}
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate)
	{
	printf("request create new stream source\n");
		live5extsource_uvc *last_uvc_source = nullptr;
		if(!_our_single_source)
		{
			printf("create new stream source\n");
			last_uvc_source = new live5extsource_uvc(_dev, env);
			if(!last_uvc_source->has_video() ||
				last_uvc_source->video_codec() != AV_CODEC_ID_H264)
			{
				delete last_uvc_source;
				last_uvc_source = nullptr;
			}
			if(last_uvc_source)
			{
				_our_single_source = (FramedSource*) H264VideoStreamFramer::createNew(env, last_uvc_source);
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

