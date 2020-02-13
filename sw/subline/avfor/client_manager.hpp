
#pragma once
/*--------------------------------------
	running for client
--------------------------------------*/
class client_manager
{
private:
	std::thread *_vreader;
	playback *_play;
	bool _bpause;
	bool _bstop;
	void notify_to_main(int code, void *ptr)
	{
		struct pe_user u;
		u._code = code;
		u._ptr = ptr;
		_int->write_user(u);
	}
	void read_video()
	{
		while(!_bstop)
		{
			pixel pix;
			int res = _play->take(pix);
			if(res < 0)
			{
				/*error or end of pixel*/
				notify_to_main(custom_code_end_video, nullptr);
				break;
			}
			else if(res == 0)	
			{
				/*no ready frame*/
				continue;
			}
			if(pix.can_take())
			{
				notify_to_main(custom_code_read_pixel, (void *)pix.clone());
				if(_play->get_master_clock() == AVMEDIA_TYPE_VIDEO)
				{
					double pts = pix.getpts();
					double *ptr = (double *)malloc(sizeof(double));
					*ptr = pts;
					notify_to_main(custo_code_presentation_tme, (void *)ptr);
				}


			}
		}	
	}

public:
	client_manager() : 
		_vreader(nullptr),
			_play(nullptr),
			_bpause(true),
			_bstop(false)
	{
		avattr attr;
		if(_avc->has_video_output_context())
		{
			attr.set(avattr_key::frame_video, "frame_video", 0, 0.0);						
			attr.set(avattr_key::width, "width", _avc->_attr.get_int(pfx_avfor_video_width,320), 0.0);
			attr.set(avattr_key::height, "height", _avc->_attr.get_int(pfx_avfor_video_height,240), 0.0);
			attr.set(avattr_key::pixel_format,"pix fmt", (int)_avc->_attr.get_int(pfx_avfor_video_format_type,(int)AV_PIX_FMT_RGB565),0.0);			
		}
		if(_avc->has_audio_output_context())
		{
			attr.set(avattr_key::frame_audio, "frame_audio", 0, 0.0);						
			attr.set(avattr_key::channel, "channel", _avc->_attr.get_int(pfx_avfor_audio_channel,1), 0.0);
			attr.set(avattr_key::samplerate, "samplerate", _avc->_attr.get_int(pfx_avfor_audio_samplingrate,48000), 0.0);
			attr.set(avattr_key::pcm_format, "pcm fmt", _avc->_attr.get_int(pfx_avfor_audio_format,(int)AV_SAMPLE_FMT_S16), 0.0);
		}
		if(attr.has_frame_any())
		{
			_play = new playback(attr, _avc->_attr.get_str(pfx_avfor_playlist).c_str());
		}
		if(_play->has( avattr_key::frame_video))
		{	
			_vreader = new std::thread([&]()->void {read_video();});
		}	
		if(_play->has( avattr_key::frame_audio))
		{
			_int->install_audio_thread(
				[&](unsigned char *pdata,int ndata)->void
				{											
						memset(pdata, 0, ndata);
						if(!_bstop)
						{
							pcm_require require;
							require.second = ndata;
							int res = _play->take(require);
							if(require.first.can_take())
							{					

								memcpy(pdata, require.first.read(), require.first.size());
								if(_play->get_master_clock() == AVMEDIA_TYPE_AUDIO)
								{
									double pts = require.first.getpts();
									double *ptr = (double *)malloc(sizeof(double));
									*ptr = pts;

									notify_to_main(custo_code_presentation_tme, (void *)ptr);
								}
							}
						}
						
				},
				attr.get_int(avattr_key::channel,1),
				attr.get_int(avattr_key::samplerate,48000),
				1024,
				(enum AVSampleFormat)attr.get_int(avattr_key::pcm_format,(int)AV_SAMPLE_FMT_S16));
				_int->run_audio_thread();

		}		
	}
	
	~client_manager()
	{
		_bstop = true;
		if(_play)
		{
			_play->resume(true);/*return running a/v thread*/			
		}
		_int->uninstall_audio_thread();/*audio thread close*/
		if(_vreader)/*video thread close*/
		{
			_vreader->join();
			delete  _vreader;
			_vreader = nullptr;
		}
		if(_play)
		{
			delete _play;
			_play = nullptr;
		}
	}
	bool can()
	{
		return _play;
	}
	void ready_to_play()
	{

		if(can())
		{

			duration_div *d= new duration_div(_play->duration());
			notify_to_main(custom_code_ready_to_play, (void *)d);
		}
	}

	enum AVMediaType master_clock()
	{
		if(can())
			return _play->get_master_clock();
		return AVMEDIA_TYPE_UNKNOWN;
	}
	void seek(double incr)
	{
		if(can())_play->seek(incr);
	}
	bool get_pause()
	{
		return _bpause;
	}
	void pause(bool p)
	{
		if(can())
		{
			bool bchange = (p != _bpause);
			_bpause = p;
			if(bchange)
			{
				if(_bpause) _play->pause();
				else 		_play->resume();	
			}		
		}
	}

};



