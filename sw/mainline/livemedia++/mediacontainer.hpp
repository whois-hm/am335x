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

		DECLARE_THROW(avformat_open_input(&_context,file, NULL, NULL) < 0, "can't open input");
		DECLARE_THROW(avformat_find_stream_info(_context, NULL) < 0, "can't find stream from input");
	}

	virtual ~mediacontainer()
	{
		for(auto  & it : _packets)
		{
			if(it.size() > 0)
			{
				it.clear();
			}
		}
		if(_context)
		{
			avformat_close_input(&_context);
		}
	}

	enum AVCodecID find_codec(enum AVMediaType type)
	{
		const AVStream *stream = find_stream(type);
		if(stream)
		{
			return stream->codecpar->codec_id;
		}
		return AV_CODEC_ID_NONE;
	};
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
			pkt = it.move();
			_packets.at(t).pop_front();
			return true;

		}

		const AVStream *s  = find_stream(t);
		if(!s)
		{
			return false;
		}
		avpacket_class packet_ref;


		while(!_eof)
		{
			ret = av_read_frame(_context, packet_ref.raw());
			if(ret < 0)
			{
				_eof = true;
				return false;
			}
			/*
			 	 	 have a packet
			 */

			if(s->index == packet_ref.raw()->stream_index)
			{
				pkt = packet_ref.move();
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
					if(_context->streams[i]->index == packet_ref.raw()->stream_index)
					{

						_packets.at(_context->streams[i]->codec->codec_type).push_back(packet_ref.move());
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
	bool eof_stream()
	{
		bool has_stream = false;
		for(auto &it : _packets)
		{
			has_stream = !it.empty() ? true : false;
			if(has_stream)
			{
				break;
			}
		}
		return _eof && (has_stream == false);

	}
	std::string filename()
	{
		return std::string(_context->filename);
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
			/*
				clear the readed datas
			 */
			for(auto &it : _packets)
			{
				it.clear();
			}

			pts += incr;
			seekpts = (int64_t) pts * AV_TIME_BASE;
			seekflags = incr < 0 ? AVSEEK_FLAG_BACKWARD : 0;

			seekpts = av_rescale_q(seekpts, AV_TIME_BASE_Q, timebase(*targetstream));



			av_seek_frame(_context, targetstream->index, seekpts, seekflags);
		}
	}
        duration_div duration(bool &has)
	{
                has = false;
		duration_div t;
                if(_context->duration != AV_NOPTS_VALUE)
                {
                    has = true;
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
            return std::make_tuple(0,0,0,0,0);/*N/A*/
	}
        double frame_start_time(bool &has)
        {
            double val = 0.0;
            has = false;
            if(_context->start_time != AV_NOPTS_VALUE)
            {
                has = true;
                int sec, us;
                sec = llabs(_context->start_time / AV_TIME_BASE);
                us = llabs(_context->start_time % AV_TIME_BASE);
                val = (double)sec + (double)(us / AV_TIME_BASE);
                if(_context->start_time >= 0)
                {
                    return val;
                }
                return -val;
            }
            /*unknown*/
            return val;
        }
        unsigned bitrate(bool &has)/*kb/s*/
        {
            has = false;
            if(_context->bit_rate)
            {
                has = true;
                return _context->bit_rate / 1000;
            }
            return 0;
        }
        unsigned number_of_frame(bool &has, const AVStream &st)
        {
            has = false;
            if(st.nb_frames > 0)
            {
                has = true;
                return st.nb_frames;
            }
            return 0;
        }
        void dump_meta(std::string *t_string = nullptr)
        {
            bool b = false;
            put_metadata("file", _context->filename, t_string);

            duration_div duration_time = duration(b);
            if(b) put_metadata("duration", duration_time, t_string);
            else  put_metadata("duration", "N/A", t_string);

            double stime = frame_start_time(b);
            if(b) put_metadata("start time", stime, t_string);
            else  put_metadata("start time", "unknown", t_string);

            unsigned bitrate_val = bitrate(b);
            if(b) put_metadata("bitrate", bitrate_val, t_string);
            else  put_metadata("bitrate", "N/A", t_string);


            /*chapters*/
            put_metadata("chapter", _context->nb_chapters, t_string);
            for(unsigned i = 0; i < _context->nb_chapters; i++)
            {
                char buffer[1024] = {0,};
                snprintf(buffer, 1000, "-chapter %d", i);
                put_metadata(buffer, t_string);
                AVChapter *ch = _context->chapters[i];
                put_metadata(buffer, t_string);

                put_metadata("-start", (double)ch->start *av_q2d(ch->time_base), t_string);
                put_metadata("-end", (double)ch->end *av_q2d(ch->time_base), t_string);
            }
            /*streams*/
            put_metadata("stream", _context->nb_streams, t_string);
            for(unsigned  i = 0 ; i < _context->nb_streams; i++)
            {
                b = false;
                AVStream *st = _context->streams[i];
                char buffer[1024] = {0,};
                snprintf(buffer, 1000, "-stream %d", i);
                put_metadata(buffer, t_string);

                put_metadata("-media type", (char *)av_get_media_type_string(st->codec->codec_type), t_string);

                put_metadata("-codec type", (char *)avcodec_get_name(st->codec->codec_id), t_string);

                unsigned nbframe = number_of_frame(b, *st);
                if(b) put_metadata("-frame count", nbframe, t_string);
                else  put_metadata("-frame count", "unknown", t_string);

                if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    put_metadata("-width", (unsigned)st->codec->width, t_string);
                    put_metadata("-height", (unsigned)st->codec->height, t_string);
                    const char *pix = av_get_pix_fmt_name(st->codec->pix_fmt);
                    if(pix) put_metadata("-pixel format", (char *)pix, t_string);
                    else    put_metadata("-pixel format", "unknown", t_string);
                }
                else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    put_metadata("-channel", (unsigned)st->codec->channels, t_string);
                    put_metadata("-samplingrate", (unsigned)st->codec->sample_rate, t_string);
                    const char *sam = av_get_sample_fmt_name(st->codec->sample_fmt);
                    if(sam) put_metadata("-sample format", (char *)sam, t_string);
                    else    put_metadata("-sample format", "unknown", t_string);
                }
            }

        }

private:
        void put_metadata(const char *key, unsigned val, std::string *t_string = nullptr)
        {
            char buffer[1024] = {0,};
            snprintf(buffer, 1000, "%d", val);
            put_metadata(key, buffer, t_string);
        }
        void put_metadata(const char *key, double val, std::string *t_string = nullptr)
        {
            char buffer[1024] = {0,};
            snprintf(buffer, 1000, "%f", val);
            put_metadata(key, buffer, t_string);
        }
        void put_metadata(const char *key, const duration_div &duration, std::string *t_string = nullptr)
        {
            char buffer[1024] = {0,};
            snprintf(buffer, 1000, "%d(h:%d m:%d s:%d)", std::get<3>(duration),
                     std::get<0>(duration),
                     std::get<1>(duration),
                     std::get<2>(duration) );
            put_metadata(key, buffer, t_string);
        }
        void put_metadata(const char *linevalue, std::string *t_string = nullptr)
        {
            put_metadata(linevalue, "", t_string);
        }
        void put_metadata(const char *key, const char *value, std::string *t_string = nullptr)
        {
            if(key && value)
            {
                char buffer[1024] = {0,};
                snprintf(buffer, 1000, "%s : %s", key, value);
                /*
                    put buffer
                 */
                if(t_string)
                {
                    t_string->append(buffer);
                    t_string->append("\n");
                    return;
                }
                /*
                    put console
                 */
                printf("%s", buffer);
                printf("\n");
            }
        }
	AVFormatContext *_context;
	std::vector<std::list<avpacket_class >>_packets;
	bool _eof;
};
