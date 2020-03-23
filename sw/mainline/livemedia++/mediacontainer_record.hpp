#pragma once
class mediacontainer_record
{
	struct encoder_video_functor
	{
		void operator()(avframe_class &frm, avpacket_class &pkt,void * c)
		{
			mediacontainer_record *record = (mediacontainer_record *)c;

			/*
				calculate presentation time
			*/
			av_packet_rescale_ts(pkt.raw(),
					record->_enc[AVMEDIA_TYPE_VIDEO]->raw()->time_base,
					record->_stm[AVMEDIA_TYPE_VIDEO]->time_base);

			/*
				stream index = video
			*/
			pkt.raw()->stream_index = record->_stm[AVMEDIA_TYPE_VIDEO]->index;

			/*
				write to context
			*/
			av_interleaved_write_frame(record->_context, pkt.raw());
		}
	};
	struct encoder_audio_functor
	{
		void operator()(avframe_class &frm, avpacket_class &pkt,void * c)
		{
			mediacontainer_record *record = (mediacontainer_record *)c;

			/*
				calculate presentation time
			*/
			av_packet_rescale_ts(pkt.raw(),
					record->_enc[AVMEDIA_TYPE_AUDIO]->raw()->time_base,
					record->_stm[AVMEDIA_TYPE_AUDIO]->time_base);

			/*
				stream index = audio
			*/
			pkt.raw()->stream_index = record->_stm[AVMEDIA_TYPE_AUDIO]->index;

			/*
				write to context
			*/
			av_interleaved_write_frame(record->_context, pkt.raw());
		}
	};
private:
	AVFormatContext *_context;
	avattr _attr[AVMEDIA_TYPE_NB];
	encoder *_enc[AVMEDIA_TYPE_NB];
	AVStream *_stm[AVMEDIA_TYPE_NB];
	uint64_t _pts[AVMEDIA_TYPE_NB];
	std::list<pixelframe> _pixel_frm;
	std::list<pcmframe> _pcm_frm;
	bool attr_hasvideo_stream()
	{
		return (	/*encode parameters check*/
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::frame_video) &&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::width) &&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::height) &&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::pixel_format)&&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::fps)&&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::bitrate)&&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::gop)&&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::max_bframe) &&
		!_attr[AVMEDIA_TYPE_VIDEO].notfound(avattr_key::video_encoderid));

	}
	bool attr_hasaudio_stream()
	{
	
		return (	/*encode parameters check*/
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::frame_audio) &&
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::channel) &&
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::samplerate) &&
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::pcm_format)&&
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::bitrate)&&
		!_attr[AVMEDIA_TYPE_AUDIO].notfound(avattr_key::audio_encoderid));
	}
	int make_stream(AVStream *&stm)
	{
		stm = avformat_new_stream(_context, nullptr);
		if(!stm)
		{
			return -1;
		}
		
		stm->id = _context->nb_streams-1;
		return 1;
	}
	int make_videostream_enc()
	{		
		if(!attr_hasvideo_stream())
		{
			return 0;
		}
		avattr copy ;
		copy.set(avattr_key::frame_video,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::frame_video));
		copy.set(avattr_key::width,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::width));
		copy.set(avattr_key::height,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::height));
		copy.set(avattr_key::pixel_format, 
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::pixel_format));
		copy.set(avattr_key::fps,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::fps));
		copy.set(avattr_key::bitrate,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::bitrate));
		copy.set(avattr_key::gop,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::gop));
		copy.set(avattr_key::max_bframe,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::max_bframe));
		copy.set(avattr_key::video_encoderid,
				_attr[AVMEDIA_TYPE_VIDEO].get(avattr_key::video_encoderid));

		/*
			current no open test
		*/
		_enc[AVMEDIA_TYPE_VIDEO] = new encoder(copy);
		
		if(_context->oformat->flags & AVFMT_GLOBALHEADER)
		{
			_enc[AVMEDIA_TYPE_VIDEO]->raw()->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}


		if(make_stream(_stm[AVMEDIA_TYPE_VIDEO]) < 0)
		{
			return -1;
		}
		
		return avcodec_parameters_from_context(_stm[AVMEDIA_TYPE_VIDEO]->codecpar, 
			_enc[AVMEDIA_TYPE_VIDEO]->raw()) < 0 ?
			-1 : 1;
		
	}
	int make_audiostream_enc()
	{
		if(!attr_hasaudio_stream())
		{
			return 0;
		}
		avattr copy ;
		copy.set(avattr_key::channel,
				_attr[AVMEDIA_TYPE_AUDIO].get(avattr_key::channel));
		copy.set(avattr_key::samplerate,
				_attr[AVMEDIA_TYPE_AUDIO].get(avattr_key::samplerate));
		copy.set(avattr_key::pcm_format, 
				_attr[AVMEDIA_TYPE_AUDIO].get(avattr_key::pcm_format));
		copy.set(avattr_key::bitrate,
				_attr[AVMEDIA_TYPE_AUDIO].get(avattr_key::bitrate));
		copy.set(avattr_key::audio_encoderid,
				_attr[AVMEDIA_TYPE_AUDIO].get(avattr_key::audio_encoderid));
		/*
			current no open test
		*/
		_enc[AVMEDIA_TYPE_AUDIO] = new encoder(copy);
		
		if(_context->oformat->flags & AVFMT_GLOBALHEADER)
		{
			_enc[AVMEDIA_TYPE_AUDIO]->raw()->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		if(make_stream(_stm[AVMEDIA_TYPE_AUDIO]) < 0)
		{
			return -1;
		}
		
		return avcodec_parameters_from_context(_stm[AVMEDIA_TYPE_AUDIO]->codecpar, 
				_enc[AVMEDIA_TYPE_AUDIO]->raw()) < 0 ?
			-1 : 1;
	}
	int open_io()
	{
		/*
			check context has stream , if not throw
		*/
		if(_context->nb_streams <= 0)
		{
			return -1;
		}
		if(!(_context->oformat->flags & AVFMT_NOFILE))
		{
			if(avio_open(&_context->pb, _context->filename, AVIO_FLAG_WRITE) < 0)
			{
				return -1;
			}
		}
		if(avformat_write_header(_context, nullptr) < 0)
		{
			return -1;
		}
		return 1;
	}

	int compare_ts(enum AVMediaType type)
	{
		/*0 mean that save packet for presentation  time*/
		/*1 mean that write flag directly */
		/*
			if use 2 stream. compare presentation time
		*/
		if(_enc[AVMEDIA_TYPE_VIDEO] && 
			_enc[AVMEDIA_TYPE_AUDIO])
		{
			if(type == AVMEDIA_TYPE_VIDEO)
			{
				return 0 >= av_compare_ts(_pts[AVMEDIA_TYPE_VIDEO], 
					_enc[AVMEDIA_TYPE_VIDEO]->raw()->time_base,
					_pts[AVMEDIA_TYPE_AUDIO], 
					_enc[AVMEDIA_TYPE_AUDIO]->raw()->time_base);
			}
			if(type == AVMEDIA_TYPE_VIDEO)
			{
				return 0 >= av_compare_ts(_pts[AVMEDIA_TYPE_AUDIO], 
					_enc[AVMEDIA_TYPE_AUDIO]->raw()->time_base,
					_pts[AVMEDIA_TYPE_VIDEO], 
					_enc[AVMEDIA_TYPE_VIDEO]->raw()->time_base);
			}
		}
		/*
			else 1 stream always write flag set
		*/
		return 1;
	}
	void compared_flush()
	{
		/*
			write record data base presentation time
		*/
		while(1)
		{
			bool effected = false;
			if(_pixel_frm.size() > 0 &&
				compare_ts(AVMEDIA_TYPE_VIDEO) > 0)
			{
				auto &it = _pixel_frm.front();
				it.raw()->pts = _pts[it.type()]++;
				_enc[it.type()]->encoding(it,encoder_video_functor(), this);
				_pixel_frm.pop_front();
				effected = true;
			}
			if(_pcm_frm.size() > 0 &&
				compare_ts(AVMEDIA_TYPE_AUDIO) > 0)
			{
				auto &it = _pcm_frm.front();
				it.raw()->pts = _pts[it.type()]++;
				_enc[it.type()]->encoding(it,encoder_audio_functor(), this);
				_pcm_frm.pop_front();
				effected = true;
			}
			if( !effected )
			{
				break;
			}
		}	
	}
	void last_flush()
	{
		/*
			flush loop once
			if all data can't flush,  throw the remain data for presentation time.
		*/
		compared_flush();
	}
	
public:
	mediacontainer_record(const avattr &attr_video,
		const avattr &attr_audio,
		char const *file) : 
		_context(nullptr)
	{
		_context = nullptr;
		for(int i = 0; i < AVMEDIA_TYPE_NB; i++)
		{
			_enc[i] = nullptr;
			_stm[i] = nullptr;
			_pts[i] = 0;
		}
	
		_attr[AVMEDIA_TYPE_VIDEO] = attr_video;
		_attr[AVMEDIA_TYPE_AUDIO] = attr_audio;
				
		avformat_alloc_output_context2(
			&_context, nullptr, nullptr, file);
		
		DECLARE_THROW(_context == nullptr,"can't open recording format");		
		DECLARE_THROW(make_videostream_enc() < 0, "can't make video stream");
		DECLARE_THROW(make_audiostream_enc() < 0, "can't make audio stream");
		DECLARE_THROW(open_io() < 0, "can't open io");
	}
	virtual ~mediacontainer_record()
	{
		last_flush();
		/*
			when constructor calling safety, the container file will be writed header,
			then, we write trailer always
		*/
		av_write_trailer(_context);

		for(int i = 0; i < AVMEDIA_TYPE_NB; i++)
		{
			if(_enc[i])
			{
				delete _enc[i];
			}
		}
		if(!(_context->oformat->flags & AVFMT_NOFILE))
		{
			avio_closep(&_context->pb);
		}
		avformat_free_context(_context);
	}
	void recording(pixelframe &f)
	{
		if(_enc[f.type()])
		{
			_pixel_frm.push_back(f);
			 compared_flush();
		}			
	}
	void recording(pcmframe &f)
	{
		if(_enc[AVMEDIA_TYPE_AUDIO])
		{
			_pcm_frm.push_back(f);
			compared_flush();
		}
	}
		
};
