
#pragma once
/*--------------------------------------
	running for client



--------------------------------------*/

class client_manager :
		public manager
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
		busyscheduler sc;
		while(!_bstop)
		{
			pixel pix;
			int res = _play->take((pix));
			if(res < 0)
			{
				/*error or end of pixel*/
				notify_to_main(custom_code_section_client_end_video, nullptr);
				break;
			}
			else if(res == 0)	
			{
				/*no ready frame*/
				continue;
			}

			if(pix.can_take())
			{
				notify_to_main(custom_code_section_client_read_pixel, (void *)pix.clone());
				if(_play->get_master_clock() == AVMEDIA_TYPE_VIDEO)
				{
					double pts = pix.getpts();
					double *ptr = (double *)malloc(sizeof(double));
					*ptr = pts;
					notify_to_main(custom_code_section_client_presentation_tme, (void *)ptr);
				}
			}
		}	
	}
	bool open()
	{
		avattr attr;
		int nvideo_width = 0;
		int nvideo_height = 0;
		int nvideo_format = 0;
		int naudio_channel = 0;
		int naudio_samplingrate = 0;
		int naudio_format = 0;
		int naudio_samplesize = 0;
		std::string source, id, passwd;

		_avc->get(section_client, play_source, source);
		_avc->get(section_client, authentication_id, id);
		_avc->get(section_client, authentication_password, passwd);

		_avc->get(section_client, decode_video_width, nvideo_width);
		_avc->get(section_client, decode_video_height, nvideo_height);
		_avc->get(section_client, decode_video_format, nvideo_format);

		_avc->get(section_client, decode_audio_channel, naudio_channel);
		_avc->get(section_client, decode_audio_samplingrate, naudio_samplingrate);
		_avc->get(section_client, decode_audio_format, naudio_format);
		_avc->get(section_gui, audio_samplesize, naudio_samplesize);


		if(nvideo_width > 0 &&
				nvideo_height > 0 &&
				nvideo_format >= 0)
		{
			attr.set(avattr_key::frame_video, "frame_video", 0, 0.0);						
			attr.set(avattr_key::width, "width", nvideo_width, 0.0);
			attr.set(avattr_key::height, "height", nvideo_height, 0.0);
			attr.set(avattr_key::pixel_format,"pix fmt", nvideo_format,0.0);
		}
		if(naudio_channel > 0 &&
				naudio_samplingrate > 0 &&
				naudio_samplesize > 0 &&
				naudio_format >= 0)
		{
			attr.set(avattr_key::frame_audio, "frame_audio", 0, 0.0);						
			attr.set(avattr_key::channel, "channel", naudio_channel, 0.0);
			attr.set(avattr_key::samplerate, "samplerate", naudio_samplingrate, 0.0);
			attr.set(avattr_key::pcm_format, "pcm fmt", naudio_format, 0.0);
		}
		if(!attr.has_frame_any() ||
				source.empty())
		{
			return false;
		}

		_play = new playback(attr,
				source.c_str(),
				5000,
				!id.empty() ? id.c_str() : nullptr,
				!passwd.empty() ? passwd.c_str() : nullptr);

		if(_play->has( avattr_key::frame_video))
		{	
			_vreader = new std::thread([&]()->void {read_video();});
		}	
		if(_play->has( avattr_key::frame_audio))
		{
			_int->install_audio_thread(
				[&](unsigned char *pdata,int ndata)->void
				{
				printf("call audio\n");
						memset(pdata, 0, ndata);
						if(!_bstop)
						{
							pcm_require require;
							require.second = ndata;
							_play->take(require);
							if(require.first.can_take())
							{					
								memcpy(pdata, require.first.read(), require.first.size());
								if(_play->get_master_clock() == AVMEDIA_TYPE_AUDIO)
								{
									double pts = require.first.getpts();
									double *ptr = (double *)malloc(sizeof(double));
									*ptr = pts;

									notify_to_main(custom_code_section_client_presentation_tme, (void *)ptr);
								}
							}
						}
						
				},
				naudio_channel,
				naudio_samplingrate,
				naudio_samplesize,
				(enum AVSampleFormat)naudio_format);
				_int->run_audio_thread();
		}		
		return true;
	}

public:
	client_manager(avfor_context  *avc,
			ui *interface) :
			manager(avc, interface),
			_vreader(nullptr),
			_play(nullptr),
			_bpause(true),
			_bstop(false) { }
	
	~client_manager()
	{
		_bstop = true;
		if(_play)
		{
			_play->resume(true);/*return running a/v thread(for no reference '_play'*/
		}
		_int->uninstall_audio_thread();/*audio thread close (for no reference '_play')*/
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
	virtual bool ready()
	{
		if(open())
		{
			duration_div *d= new duration_div(_play->duration());
			notify_to_main(custom_code_section_client_ready_to_play, (void *)d);
			return true;
		}
		return false;
	}

	virtual void operator()(ui_event &e)
	{
		if(e.what() == platform_event_user)
		{
			if(e.user()->_code == custom_code_section_client_end_video)
			{
				_int->update_pannel("mainwindow","display area",ui_color(0, 0, 0));
			}
			if(e.user()->_code == custom_code_section_client_read_pixel)
			{
				pixel *pix = (pixel *)(e.user()->_ptr);

				_int->update_pannel("mainwindow","display area",pix, true/*delete auto*/);
			}
			if(e.user()->_code == custom_code_section_client_presentation_tme)
			{
				double *v = (double *)(e.user()->_ptr);
				free(v);
			}
			if(e.user()->_code == custom_code_section_client_ready_to_play)
			{
				duration_div *v = (duration_div *)(e.user()->_ptr);
				delete v;
			}
			if(e.user()->_code == custom_code_section_connection_recv_command)
			{
				int seekvalue = 0;
				char str[256] = {0, };

				if(!strcmp("pause", (char *)e.user()->_ptr))
				{
					_play->pause();
				}
				else if(!strcmp("resume", (char *)e.user()->_ptr))
				{
					_play->resume();
				}
				else if(!strcmp("play", (char *)e.user()->_ptr))
				{
					_play->play();
				}
				else if(1==sscanf((char *)e.user()->_ptr, "seek %d", &seekvalue))
				{
					_play->seek((double)seekvalue);
				}
				else if(!strcmp("record stop", (char *)e.user()->_ptr))
				{
					_play->record(nullptr);
				}
				else if(1==sscanf((char *)e.user()->_ptr, "record start %s", str))
				{
					_play->record(str);
				}
			}
		}
	}
};



