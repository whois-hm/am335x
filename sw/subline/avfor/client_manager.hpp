
#pragma once
/*--------------------------------------
	running for client
--------------------------------------*/
class client_manager
{
private:
	std::thread *_vreader;
	std::thread *_areader;
	playback *_play;
	bool _bpause;
	bool _bstop;
	avfor_context *_ctx;
	ui * _ui;
	void notify_to_main(int code, void *ptr)
	{
		struct pe_user u;
		u._code = code;
		u._ptr = ptr;
		_ui->write_user(u);
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

			}
		}	
	}
	void read_audio()
	{
	}
public:
	client_manager(avfor_context *ctx,
		ui * ui) : 
		_vreader(nullptr),
			_areader(nullptr),
			_play(nullptr),
			_bpause(true),
			_bstop(false),
			_ctx(ctx),
			_ui(ui)
	{
		avattr attr;
		if(_ctx->has_video_output_context())
		{
			attr.set(avattr_key::frame_video, "frame_video", 0, 0.0);						
			attr.set(avattr_key::width, "width", _ctx->_attr.get_int(pfx_avfor_video_width,320), 0.0);
			attr.set(avattr_key::height, "height", _ctx->_attr.get_int(pfx_avfor_video_height,240), 0.0);
			attr.set(avattr_key::pixel_format,"pix fmt", (int)_ctx->_attr.get_int(pfx_avfor_video_format_type,(int)AV_PIX_FMT_RGB565),0.0);			
		}
		if(_ctx->has_audio_output_context())
		{
		}
		if(attr.has_frame_any())
		{
			_play = new playback(attr, _ctx->_attr.get_str(pfx_avfor_playlist).c_str());
		}
		if(_play->has( avattr_key::frame_video))
		{	
			_vreader = new std::thread([&]()->void {read_video();});
		}	
		if(_play->has( avattr_key::frame_audio))
		{
			_areader = new std::thread([&]()->void {read_audio();});
		}		
	}
	
	~client_manager()
	{
		_bstop = true;
		if(_play)_play->resume(true);
		if(_areader)
		{
			_areader->join();
			delete  _areader;
			_areader = nullptr;
		}
		if(_vreader)
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
		return _play && (_vreader || _areader);
	}
	void ready_to_play()
	{

		if(can())
		{

			duration_div d= _play->duration();
			int h = std::get<0>(d);
			int m = std::get<1>(d);
			int s = std::get<2>(d);
			char buf[256] = {0, };
			sprintf(buf, "duration:%02d.%02d.%02d", h, m, s);
			char *p = (char *)malloc(strlen(buf));		
			memcpy(p, buf, strlen(buf));
			notify_to_main(custom_code_ready_to_play, p);
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



