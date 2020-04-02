#pragma once
/*
 	 avpacet filtering class

 	 normally mp4 or other container stored encoded data reassingment.
 	 so, if not use directly ffmpeg's api, just getting the raw encodding data.
 	 use this class
 */
class avpacket_bsf
{

	mediacontainer *_refcontainer;/*just reference pointer*/
	AVBitStreamFilterContext *_bsf[AVMEDIA_TYPE_NB];
	unsigned _frame_idx[AVMEDIA_TYPE_NB];
	void write_pts(avpacket_class &pkt, const AVStream *strm, AVRational *timebase = nullptr)
	{

		if (pkt.raw()->pts == AV_NOPTS_VALUE) {
			//write pts
			AVRational time_base1 = strm->time_base;
			//duration between 2 frames (us)
			int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(strm->r_frame_rate);
			//Parameters
			pkt.raw()->pts = (double) (_frame_idx[strm->codec->codec_type]++ * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
			pkt.raw()->dts = pkt.raw()->pts;
			pkt.raw()->duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
		}
		if(timebase)
		{
			// convert output timebase PTS/DTS
			pkt.raw()->pts = av_rescale_q_rnd(pkt.raw()->pts, strm->time_base, *timebase, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.raw()->dts = av_rescale_q_rnd(pkt.raw()->dts, strm->time_base, *timebase, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.raw()->duration = av_rescale_q(pkt.raw()->duration, strm->time_base, *timebase);
			pkt.raw()->pos = -1;
		}
	}
	std::pair<AVBitStreamFilterContext *, const AVStream *>
	get_pair_stream_bsf(avpacket_class &pkt)
	{
		AVBitStreamFilterContext *targetfilter = nullptr;
		const AVStream *strm = nullptr;
		enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
		do
		{
			type = AVMEDIA_TYPE_VIDEO;
			if(can_bsf(type))
			{
				strm = _refcontainer->find_stream(type);
				if(strm->index == pkt.raw()->stream_index)
				{
					targetfilter = _bsf[type];
					break;
				}
			}
			type = AVMEDIA_TYPE_AUDIO;
			if(can_bsf(AVMEDIA_TYPE_AUDIO))
			{
				strm = _refcontainer->find_stream(type);
				if(strm->index == pkt.raw()->stream_index)
				{
					targetfilter = _bsf[type];
					break;
				}
			}
		}while(0);
		return std::make_pair(targetfilter, strm);
	}

	void open_bsf(const AVStream *stream)
	{
		if(stream)
		{
			if(stream->codec->codec_id == AV_CODEC_ID_H264)
			{
				/*

					 H.264 in some container format (FLV, MP4, MKV etc.) need
					"h264_mp4toannexb" bitstream filter (BSF)
					  *Add SPS,PPS in front of IDR frame
					  *Add start code ("0,0,0,1") in front of NALU
					H.264 in some container (MPEG2TS) don't need this BSF.

				 */
				_bsf[AVMEDIA_TYPE_VIDEO] = av_bitstream_filter_init("h264_mp4toannexb");
			}

			if(stream->codec->codec_id == AV_CODEC_ID_AAC)
			{
				/*
					AAC in some container format (FLV, MP4, MKV etc.) need
					"aac_adtstoasc" bitstream filter (BSF)
				*/
				_bsf[AVMEDIA_TYPE_AUDIO] = av_bitstream_filter_init("aac_adtstoasc");
			}
		}
	}
	void create_bsf()
	{
		/*
		 	 current case in mp4 container
		 */
		do
		{
			if(!_refcontainer)
			{
				break;
			}
			if(!contain_string((char const *)_refcontainer->filename().c_str(), ".mp4"))
			{
				break;
			}
			/*
			 	 if one bsf has open return true
			 */
			open_bsf(_refcontainer->find_stream(AVMEDIA_TYPE_VIDEO));
			open_bsf(_refcontainer->find_stream(AVMEDIA_TYPE_AUDIO));
		}while(0);
	}
public:
	/*case from container*/
	avpacket_bsf( mediacontainer *refcontainer) :
		_refcontainer(refcontainer)
	{
		for(unsigned i = 0; i < AVMEDIA_TYPE_NB; i++)
		{
			_bsf[i] = nullptr;
			_frame_idx[i] = 0;
		}
		/*
		 	 create success or fail judge is your by funtion 'can_bsf()'
		 */
		create_bsf();
	}
	virtual ~avpacket_bsf()
	{
		for(unsigned i = 0; i < AVMEDIA_TYPE_NB; i++)
		{
			if(_bsf[i])
				av_bitstream_filter_close(_bsf[i]);
		}

	}
	int can_bsf(enum AVMediaType type)
	{
		/*
		 	 -1 that can't bsf
		 	 0 that no stream
		 	 1 that can bsf
		 */
		if(!_refcontainer)
		{
			return -1;
		}
		if(!_refcontainer->find_stream(type))
		{
			return 0;
		}
		return _bsf[type] ? 1 : -1;
	}
	int operator()(avpacket_class &pkt/*from the mediacontainer's packet*/)
	{
		uint8_t *poutbuf = nullptr;
		int poutbuf_size = 0;
		/*
		 	 filtering junst pkt and insert pts if none
		 */
		std::pair<AVBitStreamFilterContext *, const AVStream *> _pair;

		_pair = get_pair_stream_bsf(pkt);
		if(_pair.first &&
				_pair.second)
		{
			/*
			 	 now filtering mp4 contain's codec data to just codec data
			 */
			av_bitstream_filter_filter(_pair.first,
					_pair.second->codec,
					nullptr,
					&poutbuf,
					&poutbuf_size,
					pkt.raw()->data,
					pkt.raw()->size, 0);

			pkt.fromdata(poutbuf,poutbuf_size);
			write_pts(pkt, _pair.second);
			return 1;
		}
		return -1;
	}
	int operator()(avpacket_class &pkt/*from the mediacontainer's packet*/,
			AVRational othertimebase)
	{
		uint8_t *poutbuf = nullptr;
		int poutbuf_size = 0;
		
		/*
		 	 filtering pkt and pts, dts insert base from 'othertimebase'
		 */
		std::pair<AVBitStreamFilterContext *, const AVStream *> _pair;

		_pair = get_pair_stream_bsf(pkt);
		if(_pair.first &&
				_pair.second)
		{
			/*
			 	 now filtering mp4 contain's codec data to just codec data
			 */
			av_bitstream_filter_filter(_pair.first,
					_pair.second->codec,
					nullptr,
				&poutbuf,
				&poutbuf_size,

					pkt.raw()->data,
					pkt.raw()->size, 0);
			pkt.fromdata(poutbuf,poutbuf_size);
			write_pts(pkt, _pair.second, &othertimebase);
			return 1;
		}
		return false;
	}
};
