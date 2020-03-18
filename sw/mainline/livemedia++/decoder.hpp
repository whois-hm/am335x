#pragma once

/*
 	 audio, video, decoder
 */
class decoder
{
private:
	void clear()
	{
		if(_avcontext)avcodec_free_context(&_avcontext);
		_avcontext = nullptr;
		decode = nullptr;
	}
public:
	template <typename functor>
	int decoding( avpacket_class &packet,
			functor &&_f,
			void *puser = nullptr)
	{


		int org_size = packet.raw()->size;
		unsigned char *org_ptr = packet.raw()->data;
		int ret = 0;
		int got = 0;
		int make_frame_number = 0;


		while(packet.raw()->size > 0)
		{
			ret = 0;
			got = 0;
			ret = decode(_avcontext, _frame.raw(), &got, packet.raw());
			if(ret >= 0 &&
					got)
			{
				/*
				  	  put to virtual function
				 */
				int cn = packet.raw()->size;
				unsigned char *cp = packet.raw()->data;
				packet.raw()->size = org_size;
				packet.raw()->data = org_ptr;

				_f(packet, _frame, puser);
				_frame.unref();

				make_frame_number++;
				packet.raw()->size = cn;
				packet.raw()->data = cp;
			}
			if(ret < 0)
			{
				/*
				  	 throw the frame and break
				 */

				//_frame.unref();
				break;
			}

			packet.raw()->size -= ret;
			packet.raw()->data += ret;
		}
		packet.raw()->size = org_size;
		packet.raw()->data = org_ptr;
		return make_frame_number;
	}
	bool opentest(const avattr &attr,
			char const *codec)
	{
		clear();
		return decoder_open_test(attr, codec, nullptr, nullptr);
	}
	bool opentest(const avattr &attr,
			enum AVCodecID codec)
	{
		clear();
		return decoder_open_test(attr, nullptr, &codec, nullptr);
	}
	bool opentest(const AVCodecParameters *codec)
	{
		clear();
		return decoder_open_test(avattr(), nullptr, nullptr, codec);
	}
	/*
	 	 create decoder by users parameter
	 */
	decoder()  : _avcontext(nullptr),
			_frame(avframe_class())
	{
		/*
		 	 useful opentest
		 */
	}
	decoder(const avattr &attr,
			char const *codec) : _avcontext(nullptr),
					_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(attr,
				codec,
				nullptr,
				nullptr),
				"can't alloc decoder");
	}
	/*
	 	 create decoder by users parameter
	 */
	decoder(const avattr &attr,
			enum AVCodecID codec) : _avcontext(nullptr),
					_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(attr,
				nullptr,
				&codec,
				nullptr),
				"can't alloc decoder");
	}
	/*
	 	 create decoder by codecs parameter
	 */
	decoder( const AVCodecParameters *codec) : _avcontext(nullptr),
			_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(avattr(),
				nullptr,
				nullptr,
				codec),
				"can't alloc decoder");
	}
	virtual ~decoder()
	{
		clear();
	}

private:
	bool decoder_open_test(const avattr &attr,
			const char *ccodec,
			enum AVCodecID *ncodec,
			const AVCodecParameters *pcodec)
	{
		AVCodec *avcodec = nullptr;
		AVCodecParameters *temppar = nullptr;
		do
		{
			/*
			 	 	 invalid parameter check
			 */
			if(!ccodec &&
					!ncodec &&
					!pcodec)
			{
				break;
			}

			/*
			 	 	 find codec
			 */

			if(ccodec) 				avcodec = avcodec_find_decoder_by_name(ccodec);
			else if(ncodec) 	avcodec = avcodec_find_decoder(*ncodec);
			else if(pcodec)
			{
				temppar = avcodec_parameters_alloc();
				if(temppar)
				{
					if(avcodec_parameters_copy(temppar, pcodec) >= 0)
					{
						avcodec = avcodec_find_decoder(temppar->codec_id);
					}
				}
			}

			if(!avcodec)
			{
				/*
				 	 	 can't find codec
				 */
				break;
			}
			if(avcodec->type != AVMEDIA_TYPE_AUDIO &&
					avcodec->type != AVMEDIA_TYPE_VIDEO)
			{
				//TODO current support codec type  'audio' and 'video'
				break;
			}
			_avcontext = avcodec_alloc_context3(avcodec);
			if(!_avcontext)
			{
				break;
			}

			/*
			 	 filled context
			 */
			/*
			 	 if constructor from codec parameter
			 */
			if(temppar)
			{
				if(avcodec_parameters_to_context(_avcontext, temppar) < 0)
				{
					break;
				}

			}
			/*
			 	 if constructor from codec name
			 */
			else
			{


				if(avcodec->type == AVMEDIA_TYPE_AUDIO)
				{
					if(!attr.has_frame_audio())
					{
						break;
					}

					_avcontext->channels = attr.get_int(avattr_key::channel);
					_avcontext->sample_rate = attr.get_int(avattr_key::samplerate);
					_avcontext->sample_fmt = (enum AVSampleFormat)attr.get_int(avattr_key::pcm_format);;
				}
				else if(avcodec->type == AVMEDIA_TYPE_VIDEO)
				{
					if(!attr.has_frame_video())
					{
						break;
					}

					_avcontext->width = attr.get_int(avattr_key::width);
					_avcontext->height = attr.get_int(avattr_key::height);
					_avcontext->pix_fmt = (enum AVPixelFormat)attr.get_int(avattr_key::pixel_format);
				}
			}

			/*
			 	 	 now open context
			 */
			if(avcodec_open2(_avcontext, avcodec, NULL) < 0)
			{
				break;
			}


			if(_avcontext->codec_type == AVMEDIA_TYPE_AUDIO)
				decode = avcodec_decode_audio4;
			else if(_avcontext->codec_type == AVMEDIA_TYPE_VIDEO)
				decode = avcodec_decode_video2;


			return true;
		}while(0);

		if(temppar)
		{


			avcodec_parameters_free(&temppar);
		}
		if(_avcontext)avcodec_free_context(&_avcontext);
		_avcontext = nullptr;

		return false;

	}
	std::function<int(AVCodecContext *avctx, AVFrame *frame,
			                          int *got_frame_ptr, const AVPacket *avpkt)> decode;
	AVCodecContext *_avcontext;
	avframe_class _frame;
};


