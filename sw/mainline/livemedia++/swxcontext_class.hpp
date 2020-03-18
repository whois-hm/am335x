#pragma once

/*
 	 interface class 'struct swscontext ', 'struct swrcontext' of ffmpeg
 */
class swxcontext_class final
{
private:

	bool paramter_test(pixelframe &frm,
			const avattr &out_attr)
	{

		if(out_attr.notfound(avattr_key::frame_video) ||
				out_attr.notfound(avattr_key::width) ||
				out_attr.notfound(avattr_key::height) ||
				out_attr.notfound(avattr_key::pixel_format))
		{
			/*
			 	 out attribute has no spec video
			 */
			return false;
		}
		int dump[3][2] =
		{
				{frm.raw()->width, out_attr.get_int(avattr_key::width)},
				{frm.raw()->height, out_attr.get_int(avattr_key::height)},
				{frm.raw()->format, out_attr.get_int(avattr_key::pixel_format)},
		};
		if(dump[0][0] == dump[0][1] &&
				dump[1][0] ==dump[1][1] &&
				dump[2][0] == dump[2][1])
		{
			/*
			 	 sampe input, output.
			 	 no need conversion
			 */
			return false;
		}

		_origin = std::make_tuple(dump[0][0], dump[1][0], dump[2][0]);
		_renew = std::make_tuple(dump[0][1], dump[1][1], dump[2][1]);


		return true;
	}
	bool paramter_test(pcmframe &frm,
			const avattr &out_attr)
	{
		if(out_attr.notfound(avattr_key::frame_audio) ||
				out_attr.notfound(avattr_key::channel) ||
				out_attr.notfound(avattr_key::samplerate) ||
				out_attr.notfound(avattr_key::pcm_format))
		{
			/*
			 	 out attribute has no spec audio
			 */
			return false;
		}

		int dump[3][2] =
		{
				{frm.raw()->channels, out_attr.get_int(avattr_key::channel)},
				{frm.raw()->sample_rate, out_attr.get_int(avattr_key::samplerate)},
				{frm.raw()->format, out_attr.get_int(avattr_key::pcm_format)},
		};
		if(dump[0][0] == dump[0][1] &&
				dump[1][0] ==dump[1][1] &&
				dump[2][0] == dump[2][1])
		{
			/*
			 	 sampe input, output.
			 	 no need conversion
			 */
			return false;
		}

		_origin = std::make_tuple(dump[0][0], dump[1][0], dump[2][0]);
		_renew = std::make_tuple(dump[0][1], dump[1][1], dump[2][1]);

		return AVMEDIA_TYPE_AUDIO;
	}

	void conv(pcmframe &frm)
	{
            //TODO metadata copy ?
            //ffmpeg has copy metadata funtion
            //but not copy our need data
		avframe_class _new;
		_new.raw()->channel_layout = av_get_default_channel_layout(std::get<0>(_renew));
		_new.raw()->channels = std::get<0>(_renew);
		_new.raw()->sample_rate = std::get<1>(_renew);
		_new.raw()->format = std::get<2>(_renew);
        _new.raw()->pts = frm.raw()->pts;
		_new.raw()->pkt_pts = frm.raw()->pkt_pts;
        _new.raw()->pkt_dts = frm.raw()->pkt_dts;
        _new.raw()->key_frame = frm.raw()->key_frame;
        _new.raw()->repeat_pict = frm.raw()->repeat_pict;
        _new.raw()->best_effort_timestamp = frm.raw()->best_effort_timestamp;
        _new.raw()->pkt_pos = frm.raw()->pkt_pos;
        _new.raw()->pkt_duration = frm.raw()->pkt_duration;
        _new.raw()->pkt_size = frm.raw()->pkt_size;



		DECLARE_THROW(swr_convert_frame((struct SwrContext *)_lock_context, _new.raw(), frm.raw()),
				"can't swr_convert");
		frm = *_new.raw();

	}
	void conv(pixelframe &frm)
	{

		avframe_class _new;
		_new.raw()->width = std::get<0>(_renew);
		_new.raw()->height = std::get<1>(_renew);
		_new.raw()->format = std::get<2>(_renew);
        _new.raw()->pts = frm.raw()->pts;
		_new.raw()->pkt_pts = frm.raw()->pkt_pts;
        _new.raw()->pkt_dts = frm.raw()->pkt_dts;
        _new.raw()->key_frame = frm.raw()->key_frame;
        _new.raw()->repeat_pict = frm.raw()->repeat_pict;
        _new.raw()->best_effort_timestamp = frm.raw()->best_effort_timestamp;
        _new.raw()->pkt_pos = frm.raw()->pkt_pos;
        _new.raw()->pkt_duration = frm.raw()->pkt_duration;
        _new.raw()->pkt_size = frm.raw()->pkt_size;

		DECLARE_THROW(av_frame_get_buffer(_new.raw(), 0),
				"can't av_frame_get_buiffer");
		DECLARE_THROW(0 >= sws_scale((struct SwsContext *)_lock_context,
				frm.raw()->data,
				frm.raw()->linesize,
				0,
				frm.raw()->height,
				_new.raw()->data,
				_new.raw()->linesize),
				"can't sws_scale");


		//TODO metadata copy ?

		frm =  *_new.raw();


	}
	void lock ( pixelframe &frm,
			const avattr &out_attr)
	{
		if(_c)
		{

			_lock_context = livemedia_pp::ref()->lock_filtercontext(std::make_pair(_origin, _renew), frm.type());
			conv(frm);
		}
	}
	void lock(pcmframe &frm,
			const avattr &out_attr)
	{
		if(_c)
		{
			_lock_context = livemedia_pp::ref()->lock_filtercontext(std::make_pair(_origin, _renew), frm.type());
			conv(frm);
		}
	}
	void unlock()
	{
		livemedia_pp::ref()->unlock_filtercontext(_lock_context);
	}
public:
	void *_lock_context;
	bool _c;
	triple_int _origin;
	triple_int _renew;
	swxcontext_class() = delete;
	/*
	 	 	 pixelframe's constructor
	 */
	swxcontext_class(pixelframe &frm,
			const avattr &out_attr) : _lock_context(nullptr),
				_c(false)
	{
		_c = paramter_test(frm, out_attr);
		lock(frm, out_attr);
	}
	/*
	 	 	 pcmframe's constructor
	 */
	swxcontext_class(pcmframe &frm,
			const avattr &out_attr) : _lock_context(nullptr),
				_c(false)
	{
		_c = paramter_test(frm, out_attr);
		lock(frm, out_attr);
	}
	~swxcontext_class()
	{
		unlock();
	}
};



