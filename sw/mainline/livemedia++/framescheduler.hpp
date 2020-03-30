#pragma once


/*
 	 buffered frames

 	 for speed up buffering, we none check input attribute frame
 	 you must be exactly match attribute frame input

	normally use audio / video -> 'avframebuffering'
 */



template <typename _Tframe_pixel, typename _Tframe_pcm> class framebuffering_type
{
protected:

	/*
	 	 	 you can get this max pcm size, at once calling
	 */
	template <int audio_size = 10000>
	struct audio_array_buffer
	{
		uint8_t _buffer[audio_size];
		int _index, _max;
		triple_int audio_output_attr;
		audio_array_buffer(const avattr &a):_index(0), _max(audio_size)
		{
			std::get<0>(audio_output_attr) = a.get_int(avattr_key::channel);
			std::get<1>(audio_output_attr) = a.get_int(avattr_key::samplerate);
			std::get<2>(audio_output_attr) = a.get_int(avattr_key::pcm_format);
		}
	};

	avattr _attr;

	/*
	  	  video require fixed size always.
	  	  so operation list only
	 */
	std::list<_Tframe_pixel>_pixelframe;

	/*
		 	 audio buffer require variable size,
		 	 so operation is not class but array
	 */
	std::pair<audio_array_buffer<>,
	std::list<_Tframe_pcm>> _pcmframe;
protected:
	virtual void usedframe(_Tframe_pixel &rf) { }
	virtual void usedframe(_Tframe_pcm &rf, int size_from_array = 0) { }
	virtual void usingframe(_Tframe_pixel &rf) { }
	virtual void usingframe(_Tframe_pcm &rf) { }
public:
	framebuffering_type(const avattr &attr) :
		_attr(attr),
		_pixelframe(std::list<_Tframe_pixel>()),
		_pcmframe(std::make_pair(framebuffering_type::audio_array_buffer<>(attr),
				std::list<_Tframe_pcm>()))
	{

	}
	virtual ~framebuffering_type()
	{
		data_clear();
	}
	void data_clear()
	{
		_pixelframe.clear();
		_pcmframe.first._index = 0;
		_pcmframe.second.clear();
	}
	void operator <<(const _Tframe_pixel &rf)
	{

		_pixelframe.push_back(rf);
		usingframe(_pixelframe.back());
	}
	void operator <<(const _Tframe_pcm &rf)
	{
		_Tframe_pcm &urf = const_cast<_Tframe_pcm &>(rf);
		int pcm_length = 0;
		int audio_remain_size = 0;


		pcm_length = urf.len();

		audio_remain_size = _pcmframe.first._max -_pcmframe.first._index;
		if(audio_remain_size >= pcm_length)
		{
			//printf("pcm(%d) used in array remain = %d length = %d\n", urf.print(), audio_remain_size, pcm_length);
			urf.data_copy(_pcmframe.first._buffer + _pcmframe.first._index, pcm_length);
			_pcmframe.first._index += pcm_length;
			usedframe(urf, pcm_length);
			return;
		}
		/*
		 	 array was full
		 */
		// printf("pcm(%d) insert in list remain = %d length = %d\n", urf.print(), audio_remain_size, pcm_length);
		_pcmframe.second.push_back(rf);
		usingframe(_pcmframe.second.back());
	}
	pixel &operator >>(pixel &pf)
	{
		do
		{
			if(_pixelframe.empty())
			{
				break;
			}
			auto &it = _pixelframe.front();
			it >> pf;
			usedframe(it);
			_pixelframe.pop_front();
		}while(0);

		return pf;
	}

	pcm_require &operator >>(pcm_require &pf)
	{
		/*
		 	 first check appropriate return size
		 */
		int return_size = std::min(pf.second, _pcmframe.first._index);
		if(return_size <= 0)
		{
			/*
			 	 	 no have data in buffer
			 */
			return pf;
		}

		/*
		 	 	 copy to
		 */
		pf.first.write(_pcmframe.first._buffer, return_size);
		pf.first.set(std::tuple<int ,
				int ,
				int ,
				enum AVSampleFormat>
				(std::get<0>(_pcmframe.first.audio_output_attr),
						std::get<1>(_pcmframe.first.audio_output_attr),
						return_size / av_get_bytes_per_sample((enum AVSampleFormat)std::get<2>(_pcmframe.first.audio_output_attr)),
				(enum AVSampleFormat)std::get<2>(_pcmframe.first.audio_output_attr)));
		/*
		 	 	 realign remian data
		 */
		
		_pcmframe.first._index -= return_size;
	//	 printf("user pcm get %d size remain = %d\n", return_size, _pcmframe.first._max -_pcmframe.first._index);
		if(_pcmframe.first._index > 0)
		{
			memcpy(_pcmframe.first._buffer, _pcmframe.first._buffer + return_size, _pcmframe.first._index);
		}
		while(!_pcmframe.second.empty())
		{
			int length = _pcmframe.second.front().len();
			if((length<= (_pcmframe.first._max -_pcmframe.first._index)))
			{
//				printf("pcm(%d) move to array in list remain = %d length = %d\n", _pcmframe.second.front().print(), _pcmframe.first._max -_pcmframe.first._index, length);
				_pcmframe.second.front().data_copy(_pcmframe.first._buffer + _pcmframe.first._index, length);
				_pcmframe.first._index += length;
				usedframe(_pcmframe.second.front(), length);
				_pcmframe.second.pop_front();
			}
			else
			{
				break;
			}
		}
		return pf;
	}
};


typedef framebuffering_type<pixelframe, pcmframe> avframebuffering;




class pixelframe_presentationtime : public pixelframe
{
protected:
	double _pts;
	pixelframe_presentationtime(const pixelframe &f) : pixelframe(f), _pts(0.0){}
	virtual ~pixelframe_presentationtime(){}
public:
	virtual double getpts() = 0;
};
class pcmframe_presentationtime : public pcmframe
{
protected:
	double _pts;
	pcmframe_presentationtime(const pcmframe &f) : pcmframe(f), _pts(0.0){}
	virtual ~pcmframe_presentationtime(){}
        virtual double guesspts() = 0;
public:
	virtual double getpts() = 0;

};

/* Skip or repeat the frame. Take delay into account
 FFPlay still doesn't "know if this is the best guess." */
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define AUDIO_DIFF_AVG_NB 20
#define AUDIO_AVG_DIFF_THRESHOLD	0.0
# define SAMPLE_CORRECTION_PERCENT_MAX 10
/*
 	 	 scheduling frame based presentation time
 */

template <typename _type_pixelframe, typename _type_pcmframe> class avframescheduler :
		public framebuffering_type<_type_pixelframe, _type_pcmframe>
{
private:
	enum AVMediaType _masterclock;
	enum _video_type
	{
		last_presentation_time,
		current_presentation_time,
		last_delay_time,
		clock_base_frame_pts_timer,
		global_timer,
		max
	};
	double _video[_video_type::max];

	int _audio_bps;
	double _audio_last_pts;
    int _audiodiff_avgcount;
    double _audiodiff_cum;
    double _audiodiff_avgcoef;

	double master_clock_pts()
	{
		if(_masterclock == AVMEDIA_TYPE_VIDEO)
		{
			double delta;
			delta = (av_gettime() - _video[_video_type::global_timer]) / 1000000.0;
			return _video[_video_type::current_presentation_time] + delta;
		}
		else if(_masterclock == AVMEDIA_TYPE_AUDIO)
		{
			return _audio_last_pts - (framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first._index *
					std::get<0>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr))/_audio_bps;
		}
		return 0.0;
	}

	virtual void usedframe( _type_pixelframe &rf)
	{
		double delay = 0.0;
		double diff_pts  = 0.0;
		double threshold = 0.0;
		double act_delay = 0.0;
		double masters_clock = 0.0;

		/*
		 	 	 get delay from current frame - last frame
		 */

		_video[_video_type::current_presentation_time] = rf.getpts();
		_video[_video_type::global_timer] = (double)av_gettime();
		delay = _video[_video_type::current_presentation_time] - _video[_video_type::last_presentation_time];


		/*
		 	 	 min delay => 0 sec && delay <= 1 sec
		 */
		if(delay <= 0 || delay >= 1.0)
		{

			delay = _video[_video_type::last_delay_time];
		}
		_video[_video_type::last_presentation_time] = _video[_video_type::current_presentation_time];
		_video[_video_type::last_delay_time] = delay;

		/*
		 	 	 master clock syncronize
		 */
		masters_clock = master_clock_pts();
		diff_pts = _video[_video_type::current_presentation_time] - masters_clock;

		/*
		 	 	threshold
		 */
		threshold = delay > AV_SYNC_THRESHOLD ? delay : AV_SYNC_THRESHOLD;
		if(fabs(diff_pts) < AV_NOSYNC_THRESHOLD)
		{
			if(diff_pts <= - threshold) delay = 0;
			else if(diff_pts >=  threshold) delay = 2* delay;
			else
			{
				delay += -diff_pts;
			}
		}

		/*
				start sync
		 */
		_video[_video_type::clock_base_frame_pts_timer] += delay;

		double currenttimer = (double)av_gettime() / 1000000.0;
		act_delay = _video[_video_type::clock_base_frame_pts_timer] - currenttimer;

		if(act_delay < 0.010)
		{
			/*minimum delay 10ms*/
			act_delay = 0.010;
		}

		usleep((act_delay * 1000 + 0.5) * 1000);
	}
	virtual void usedframe( _type_pcmframe &rf, int size_from_array = 0)
	{

		_audio_last_pts = rf.guesspts() /*input pts */ +
				(size_from_array)/*input frame size */ /
				(double)_audio_bps;
	}
	virtual void usingframe( _type_pixelframe &rf) = 0;
	virtual void usingframe( _type_pcmframe &rf) = 0;


public:
	avframescheduler(const avattr &attr) :
		framebuffering_type<_type_pixelframe, _type_pcmframe>(attr),
		_masterclock(AVMEDIA_TYPE_UNKNOWN)
	{
		data_clear();
	}
	void data_clear()
	{
		_video[last_presentation_time] =
		_video[current_presentation_time] =
		_video[last_delay_time] =
		_video[clock_base_frame_pts_timer] =
		_video[global_timer] = 0.0;

		/*
		 	 	 initialize value video
		 */
		_video[last_delay_time] = 40e-3;
		_video[clock_base_frame_pts_timer] = (double) (av_gettime() / 1000000.0);;
		_video[global_timer] = (double) av_gettime();

		/*
		 	 initialize value audio
		 */
	    _audiodiff_avgcount = 0;
	    _audiodiff_cum = 0.0;
	    _audiodiff_avgcoef = 0.0;
		_audio_last_pts = 0.0;
		/*channel * samplingrate * sample number*/
		_audio_bps = std::get<0>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr) *
				std::get<1>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr) *
				av_get_bytes_per_sample((enum AVSampleFormat)std::get<2>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr));
		framebuffering_type<_type_pixelframe, _type_pcmframe>::data_clear();
	}
	void set_clock_master(enum AVMediaType masterclock)
	{
		_masterclock = masterclock;
	}
	enum AVMediaType get_clock_master(double *presentation_time = nullptr)
	{
		if(presentation_time)
		{
			/*master's clock pts*/
			*presentation_time =	master_clock_pts();
		}
		return  _masterclock;
	}

	pixel &operator >>(pixel &pf)
	{
            if(!framebuffering_type<_type_pixelframe, _type_pcmframe>::_pixelframe.empty())
            {
                pf.setpts(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pixelframe.front().getpts());
            }

            return framebuffering_type<_type_pixelframe, _type_pcmframe>::operator >>(pf);
	}
	pcm_require &operator >>(pcm_require &pf)
	{
	    double diff = 0.0;
	    double avg_diff = 0.0;
	    int manipulate_size = 0;
	    int min_size = 0;
	    int max_size = 0;
		pcm_require &dump = framebuffering_type<_type_pixelframe, _type_pcmframe>::operator >>(pf);
		if(!dump.first.can_take())
		{
			return dump;
		}
		/*
		 	  equally same pts value from mastclock
		 	  by manipulate readed pcm data
		 */
		do
		{
			double audio_current_clock =
					_audio_last_pts - (framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first._index * std::get<0>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr))/
					_audio_bps;

                        pf.first.setpts(audio_current_clock);


			diff = audio_current_clock - master_clock_pts();
			if(diff >= AV_NOSYNC_THRESHOLD)
			{
				/* difference is TOO big; reset diff stuff */
				_audiodiff_avgcount = 0;
				_audiodiff_cum = 0;
				break;
			}
			/*find weight */
			/*
			 	 	 So we're going to use a fractional coefficient, say c, and sum the differences like this: diff_sum = new_diff + diff_sum*c.
			 	 	 When we are ready to find the average difference, we simply calculate avg_diff = diff_sum * (1-c).
			 */
			if(_audiodiff_avgcount < AUDIO_DIFF_AVG_NB)
			{
				/*not yet avg*/
				_audiodiff_cum = diff + _audiodiff_avgcoef * _audiodiff_cum;
				_audiodiff_avgcount ++;
				break;
			}
			/*now found average*/
			avg_diff = _audiodiff_cum * (1.0 - _audiodiff_avgcoef);
			if(avg_diff < AUDIO_AVG_DIFF_THRESHOLD)
			{
				break;
			}
			/*
			 	 	 Remember that audio_length * (sample_rate * # of channels * 2) is the number of samples in audio_length seconds of audio.
			 	 	 Therefore, number of samples we want is going to be the number of samples we already have plus or minus the number of samples that correspond to the amount of time the audio has drifted
			 */
		  manipulate_size = dump.first.size() + (int) (diff * _audio_bps);
		  min_size = dump.first.size() * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
		  max_size = dump.first.size() * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);
		  if(manipulate_size < min_size)
		  {
			  manipulate_size = min_size;
		  }
		  else if(manipulate_size > max_size)
		  {
			  manipulate_size = max_size;
		  }
		  manipulate_size -= dump.first.size();
		}while(0);

		/*
		 	 	 finally  size change from sync size
		 */

		if(manipulate_size < 0)
		{
			/*
					 case overflow
			 */
			int drop_size = std::abs(manipulate_size);
			int dropped_len = dump.first.size() - drop_size;

			uint8_t _newpcm[dropped_len];
			dump.first.copyto(_newpcm, dropped_len);
			dump.first.write(_newpcm, dropped_len);
		}
		else if(manipulate_size > 0)
		{
			/*
			 	 	 case underflow
			 	 	 this case do increase the buffer,
			 	 	 so we can not avoid realloc the memory
			 */

			int append_size = std::abs(manipulate_size);
			int appended_len = dump.first.size() + append_size;
			int bytesamplesize = av_get_bytes_per_sample((enum AVSampleFormat)std::get<2>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr));
			const uint8_t * lastsample = dump.first.read() + (dump.first.size() - bytesamplesize);


			uint8_t new_pcm_buffer[appended_len];
			uint8_t *ptr = new_pcm_buffer;
			dump.first.copyto(ptr, dump.first.size());

			while(append_size--)
			{
				memcpy(ptr + dump.first.size() + append_size, lastsample, bytesamplesize);
			}
			dump.first.write(ptr, appended_len);
		}
		dump.first.set(std::tuple<int ,
				int ,
				int ,
				enum AVSampleFormat>
				(std::get<0>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr),
						std::get<1>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr),
						dump.first.size() / av_get_bytes_per_sample((enum AVSampleFormat)std::get<2>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr)),
				(enum AVSampleFormat)std::get<2>(framebuffering_type<_type_pixelframe, _type_pcmframe>::_pcmframe.first.audio_output_attr)));

		return dump;
	}
	virtual ~avframescheduler()
	{

	}
};
