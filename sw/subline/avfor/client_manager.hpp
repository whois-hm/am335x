
#pragma once
/*--------------------------------------
	running for client



--------------------------------------*/

unsigned  indx = 0;
class pixel_time_debug :
		public pixel
{
public:
	unsigned _createtime;
	unsigned _main_send_time;
	unsigned _main_recv_time;
	unsigned _draw_start_time;
	unsigned _draw_end_time;
	unsigned id;
	pixel_time_debug() : pixel(),
			_createtime(sys_time_c()),
			_main_send_time(0),
			_main_recv_time(0),
			_draw_start_time(0),
			_draw_end_time(0),
			id(indx++)
	{

	}
	pixel_time_debug(const pixel_time_debug &rhs ) :
		pixel(dynamic_cast<const pixel &>(rhs)),
		_createtime(rhs._createtime),
		_main_send_time(rhs._main_send_time),
		_main_recv_time(rhs._main_recv_time),
		_draw_start_time(rhs._draw_start_time),
		_draw_end_time(rhs._draw_end_time),
		id(rhs.id)
	{

	}

	virtual ~pixel_time_debug()
	{

	}
	void clip_main_send_time()
	{
		_main_send_time = sys_time_c();
	}
	void clip_main_recv_time()
	{

		_main_recv_time = sys_time_c();
	}
	void clip_draw_start_time()
	{
		_draw_start_time = sys_time_c();
	}
	void clip_draw_end_time()
	{
		_draw_end_time = sys_time_c();
	}
	pixel_time_debug *clone()
	{
		return new pixel_time_debug(*this);
	}
	void print_clip()
	{
		printf("id : %d -> (createtime ~ endtime : %u create ~ taketime : %u main_send ~ main_recv : %u draw_start ~ draw_end : %u) \n",
				id,
				_draw_end_time - _createtime,
				_main_send_time - _createtime,
				_main_recv_time - _main_send_time,
				_draw_end_time - _draw_start_time
				);

	}

};
unsigned clock_pre = 0;
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
			pixel_time_debug pix;
			int res = _play->take(dynamic_cast<pixel &>(pix));
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
				printf("take..\n");
				pix.clip_main_send_time();
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
	client_manager(avfor_context  *avc,
			ui *interface) :
			manager(avc, interface),
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
			printf("audio load\n");
			_int->install_audio_thread(
				[&](unsigned char *pdata,int ndata)->void
				{
				printf("audio load calll\n");
						memset(pdata, 0, ndata);
						if(!_bstop)
						{
							pcm_require require;
							require.second = ndata;
							int res = _play->take(require);
							if(require.first.can_take())
							{					
								double pts = require.first.getpts();
												//printf("audio pts = %f\n", pts);
								memcpy(pdata, require.first.read(), require.first.size());
								if(_play->get_master_clock() == AVMEDIA_TYPE_AUDIO)
								{
									double pts = require.first.getpts();
									double *ptr = (double *)malloc(sizeof(double));
									*ptr = pts;

									notify_to_main(custo_code_presentation_tme, (void *)ptr);
								}
							}
							else
							{
								printf("audio no data %d\n", res);
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
	virtual bool ready()
	{
		if(_play)
		{
			duration_div *d= new duration_div(_play->duration());
			notify_to_main(custom_code_ready_to_play, (void *)d);
			return true;
		}
		return false;
	}

	virtual void operator()(ui_event &e)
	{
		if(e.what() == platform_event_touch)
		{
			if(e.effected_widget_hint("close btn")) 		_int->set_loopflag(false);
			else if(e.effected_widget_hint("left btn")) 	_play->seek(-10.0);
			else if(e.effected_widget_hint("right btn")) 	_play->seek(10.0);
			else if(e.effected_widget_hint("play btn"))
			{
				if(_bpause) 	_play->resume();
				else 			_play->pause();
			}
		}
		if(e.what() == platform_event_user)
		{
			if(e.user()->_code == custom_code_end_video)
			{
				_int->update_label("mainwindow","title","play:end",ui_color(255,255,255));
				_int->update_label("mainwindow","time lable pts","pts:00.00.00",ui_color(255,255,0));
				_int->update_label("mainwindow","time lable duration","duration:00.00.00",ui_color(0,255,255));
				_int->update_pannel("mainwindow","display area",ui_color(100, 100, 100));
			}
			if(e.user()->_code == custom_code_read_pixel)
			{

				 pixel_time_debug *pix = (pixel_time_debug *)(e.user()->_ptr);
				pix->clip_main_recv_time();
				pix->clip_draw_start_time();
				pixel *a = (pixel *)pix;

				_int->update_pannel("mainwindow","display area",a, false);
				pix->clip_draw_end_time();
				pix->print_clip();
				delete pix;

//				pixel *pix = (pixel *)(e.user()->_ptr);
//				_int->update_pannel("mainwindow","display area",pix, true);
			}
			if(e.user()->_code == custo_code_presentation_tme)
			{
				double *v = (double *)(e.user()->_ptr);
				double pts = *v;
				int h, m,s;
				unsigned ms_total = pts * 1000;
				s = ms_total / 1000;
				m = s / 60;
				h = m / 60;
				s = s % 60;
				m = m % 60;
				char buf[256] = {0, };
				sprintf(buf, "pts:%02d.%02d.%02d", h, m, s);
				_int->update_label("mainwindow","time lable pts",buf,ui_color(255,255,0));
				free(v);
			}
			if(e.user()->_code == custom_code_ready_to_play)
			{
				duration_div *v = (duration_div *)(e.user()->_ptr);
				int h = std::get<0>(*v);
				int m = std::get<1>(*v);
				int s = std::get<2>(*v);
				char buf[256] = {0, };
				sprintf(buf, "duration:%02d.%02d.%02d", h, m, s);
				_int->update_label("mainwindow","title","play:ready to play",ui_color(255,255,255));
				_int->update_label("mainwindow","time lable duration",buf,ui_color(0,255,255));
				delete v;

			}
		}

	}
};



