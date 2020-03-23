#pragma once


class encoder
{
	AVCodecContext *_avcontext;
	uint64_t _pts;
	avpacket_class _pkt;	
	std::function<int(AVCodecContext *,
		AVPacket *,
		AVFrame *, 
		int *)>
		encode;
		
private:
	void clear(AVCodecContext *&target)
	{
		if(target)
		{
			avcodec_free_context(&target);
			target = nullptr;
		}
	}
	bool encoder_open_test( const avattr &attr, AVCodecContext *&target)
	{		
		
		enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
		AVCodec *codec = nullptr;
		AVRational fps;
		clear(target);
		
		if(!attr.notfound(avattr_key::frame_video) &&
			!attr.notfound(avattr_key::width) &&
			!attr.notfound(avattr_key::height) &&
			!attr.notfound(avattr_key::pixel_format)&&
			!attr.notfound(avattr_key::fps)&&
			!attr.notfound(avattr_key::bitrate)&&
			!attr.notfound(avattr_key::gop)&&
			!attr.notfound(avattr_key::max_bframe) &&
			!attr.notfound(avattr_key::video_encoderid))
		{
			type = AVMEDIA_TYPE_VIDEO;
		}
		/*
			todo AVMEDIA_TYPE_AUDIO
		*/
		if(type == AVMEDIA_TYPE_UNKNOWN)
		{
			return false;
		}

		do
		{

			if(type == AVMEDIA_TYPE_VIDEO)
			{
				codec = avcodec_find_encoder((enum AVCodecID)attr.get_int(avattr_key::video_encoderid));
				if(!codec)
				{
					break;
				}
				target = avcodec_alloc_context3(codec);
				if(!target)
				{
					break;
				}

					fps.num = 1;
					fps.den = attr.get_int(avattr_key::fps);
					target->width = attr.get_int(avattr_key::width);
					target->height = attr.get_int(avattr_key::height);
					target->bit_rate = attr.get_int(avattr_key::bitrate);
					target->time_base = fps;
					target->gop_size = attr.get_int(avattr_key::gop);
					target->max_b_frames = attr.get_int(avattr_key::max_bframe);
					target->pix_fmt = (enum AVPixelFormat)attr.get_int(avattr_key::pixel_format);
					encode = avcodec_encode_video2;
				

				if(avcodec_open2(target, codec, nullptr))
				{
					break;
				}
			}
			return true;
		}while(0);
		clear(target);
		return false;
	}
public:
	encoder() : 
		_avcontext(nullptr),
		_pts(0),
		_pkt(avpacket_class())
	{
		/*for open test user*/
	}
	encoder(  const avattr &attr) : 
		_avcontext(nullptr),
		_pts(0),
		_pkt(avpacket_class())
	{
		DECLARE_THROW(!encoder_open_test(attr, _avcontext),
				"can't alloc encoder");
	}
	int parameter_open_test( const avattr &attr)
	{
		AVCodecContext *test_avcontext = nullptr;
		bool res = false;
	    res = encoder_open_test(attr, test_avcontext);
		clear(test_avcontext);
		return res ? 0 : -1;
	}
	AVCodecContext *raw()
	{
		return _avcontext;
	}
	virtual ~encoder()
	{
		clear(_avcontext);
	}

	template <typename functor>
	int encoding( avframe_class &frm,
			functor &&_f,
			void *puser = nullptr)
	{
		if(!_avcontext)
		{
			return -1;
		}
		/*
			none check valid parameter in frm.raw(), return error perhaps in encode funtion
		*/
		int got = 0;
		int encoded = 0;

		if(_pts <= 0) _pts = 1;

		if(frm.raw()->pts == AV_NOPTS_VALUE)
		{
			frm.raw()->pts = _pts++;
		}
		encoded = encode(_avcontext, _pkt.raw(), frm.raw(), &got);	
		if(got&&
			encoded >= 0)
		{
			_f(frm, _pkt, puser);
			_pkt.unref();
		}

		if(got) return 1;
		return 0;
	}

};
