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
	void clear()
	{
		if(_avcontext)
		{
			avcodec_free_context(&_avcontext);
			_avcontext = nullptr;
		}
	}
	bool encoder_open_test( const avattr &attr)
	{		
		enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
		AVCodec *codec = nullptr;
		AVRational fps;
		if(attr.notfound(avattr_key::frame_video) &&
			attr.notfound(avattr_key::width) &&
			attr.notfound(avattr_key::height) &&
			attr.notfound(avattr_key::pixel_format)&&
			attr.notfound(avattr_key::fps)&&
			attr.notfound(avattr_key::bitrate)&&
			attr.notfound(avattr_key::gop)&&
			attr.notfound(avattr_key::max_bframe) && 
			attr.notfound(avattr_key::encoderid))
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
			codec = avcodec_find_encoder((enum AVCodecID)attr.get_int(avattr_key::encoderid));
			if(!codec)
			{
				break;
			}
			_avcontext = avcodec_alloc_context3(codec);
			if(!_avcontext)
			{
				break;
			}
			if(type == AVMEDIA_TYPE_VIDEO)
			{
				fps.num = 1;
				fps.den = attr.get_int(avattr_key::fps);
				_avcontext->width = attr.get_int(avattr_key::width);
				_avcontext->height = attr.get_int(avattr_key::height);
				_avcontext->bit_rate = attr.get_int(avattr_key::bitrate);
				_avcontext->time_base = fps;
				_avcontext->gop_size = attr.get_int(avattr_key::gop);
				_avcontext->max_b_frames = attr.get_int(avattr_key::max_bframe);
				_avcontext->pix_fmt = (enum AVPixelFormat)attr.get_int(avattr_key::pixel_format);
				encode = avcodec_encode_video2;
			}

			if(avcodec_open2(_avcontext, codec, nullptr))
			{
				break;
			}
			return true;
		}while(0);
		clear();
		return false;
	}
public:
	encoder(  const avattr &attr) : 
		_avcontext(nullptr),
		_pts(0),
		_pkt(avpacket_class())
	{
		throw_if ti;
		ti(!encoder_open_test(attr),
				"can't alloc encoder");
	}
	virtual ~encoder()
	{
		clear();
	}

	template <typename functor>
	int encoding( avframe_class &frm,
			functor &&_f,
			void *puser = nullptr)
	{
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
			_pkt = avpacket_class();/*unref*/
		}

		if(got) return 1;
		return 0;
	}

};
