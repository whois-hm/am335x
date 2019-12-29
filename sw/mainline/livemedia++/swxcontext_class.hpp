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
		throw_if ti;
		avframe_class _new;
		_new.raw()->channel_layout = av_get_default_channel_layout(std::get<0>(_renew));
		_new.raw()->channels = std::get<0>(_renew);
		_new.raw()->sample_rate = std::get<1>(_renew);
		_new.raw()->format = std::get<2>(_renew);


		//TODO metadata copy ?
		ti(swr_convert_frame((struct SwrContext *)_lock_context, _new.raw(), frm.raw()),
				"can't swr_convert");
		frm = *_new.raw();

	}
	void conv(pixelframe &frm)
	{
		throw_if ti;

		avframe_class _new;
		_new.raw()->width = std::get<0>(_renew);
		_new.raw()->height = std::get<1>(_renew);
		_new.raw()->format = std::get<2>(_renew);

		ti(av_frame_get_buffer(_new.raw(), 0),
				"can't av_frame_get_buiffer");
		ti(0 >= sws_scale((struct SwsContext *)_lock_context,
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



