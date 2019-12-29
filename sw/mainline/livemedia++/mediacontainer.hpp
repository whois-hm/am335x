#pragma once

/*
 	 interface mediafile of ffmpeg
 */
class mediacontainer
{
public:
	mediacontainer(char const *file) : _context(nullptr),
	_packets(AVMEDIA_TYPE_NB),
	_eof(false)
	{
		throw_if ti;
		ti(avformat_open_input(&_context,file, NULL, NULL) < 0, "can't open input");
		ti(avformat_find_stream_info(_context, NULL) < 0, "can't find stream from input");
	}

	virtual ~mediacontainer()
	{
		if(_context)
		{
			avformat_close_input(&_context);
		}
	}
	const AVStream * find_stream(enum AVMediaType type)
	{
		for(unsigned  i = 0 ; i < _context->nb_streams; i++)
		{
			if(_context->streams[i]->codec->codec_type == type)
			{
				return _context->streams[i];
			}
		}
		return NULL;
	}
	bool read_packet(enum AVMediaType t, avpacket_class &pkt)
	{
		int ret = 0;
		if(_packets.at(t).size() > 0)
		{
			auto &it = _packets.at(t).front();
			pkt = it;
			_packets.at(t).pop_front();
			return true;

		}

		const AVStream *s  = find_stream(t);
		if(!s)
		{
			return false;
		}
		AVPacket packet_ref;
		av_init_packet(&packet_ref);

		while(!_eof)
		{
			ret = av_read_frame(_context, &packet_ref);
			if(ret < 0)
			{
				_eof = true;
				return false;
			}
			/*
			 	 	 have a packet
			 */

			if(s->index == packet_ref.stream_index)
			{
				pkt = packet_ref;
				return true;
			}
			else
			{
				/*
						 because not sorted packet in the ffmpeg context.
						 unlike request parameter different  packet type return.

						 we stored different  packet type and return the cache value
						 if match type parameter requested
				 */

				for(unsigned i = 0; i < _context->nb_streams; i++)
				{
					if(_context->streams[i]->index == packet_ref.stream_index)
					{

						_packets.at(_context->streams[i]->codec->codec_type).push_back(avpacket_class (packet_ref));
						break;
					}
				}
			}
		}

		return false;
	}
	bool read_packet(const AVStream *src, avpacket_class &pkt)
	{
		if(src &&
				find_stream(src->codec->codec_type))
		{
			return read_packet(src->codec->codec_type, pkt);
		}
		return false;
	}
	bool eof_stream(enum AVMediaType t)
	{
		if(find_stream(t) ==
				nullptr)
		{
			return true;
		}
		if(_eof &&
				_packets.at(t).empty())
		{
			return true;
		}
		return false;
	}
	AVRational timebase(const AVStream &s)
	{
		return s.time_base;
	}
	void seek(enum AVMediaType base, double incr, double pts)
	{
		int64_t seekpts = 0;
		int seekflags = 0;
		const AVStream *targetstream = nullptr;
		targetstream = find_stream(base);
		if(targetstream)
		{
			pts += incr;
			seekpts = (int64_t) pts * AV_TIME_BASE;
			seekflags = incr < 0 ? AVSEEK_FLAG_BACKWARD : 0;

			seekpts = av_rescale_q(seekpts, AV_TIME_BASE_Q, timebase(*targetstream));


			av_seek_frame(_context, targetstream->index, seekpts, seekflags);
		}
	}
	duration_div duration()
	{
		duration_div t;

		int64_t duration = _context->duration + 5000;
		int _h, _m, _s, _us;


		_s = duration / AV_TIME_BASE;
		_us = duration % AV_TIME_BASE;

		_m = _s / 60;
		_s = _s % 60;
		_h = _m / 60;
		_m = _m% 60;

		t = std::make_tuple(_h,
				_m,
				_s,
				(int)(_us * 100)  / AV_TIME_BASE,
				duration);
		return t;
	}
private:
	AVFormatContext *_context;
	std::vector<std::list<avpacket_class >>_packets;
	bool _eof;
};
