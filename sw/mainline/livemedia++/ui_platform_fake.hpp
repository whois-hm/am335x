
#pragma once
class ui_platform_fake : 
	public ui
{
private:
	struct WQ *_workq;
	ui_event_filter _filter;
public :
	ui_platform_fake(char const *name) : 
		ui(name, platform_on_fake)
	{
		_workq =  WQ_open(sizeof(struct platform_par) , 10);
	}
	virtual ~ui_platform_fake()
		{
		WQ_close(&_workq);
		}

virtual	bool make_window(ui_handle win_name, 
		ui_rect &&rect)
		{
			return true;
		}
		
virtual	bool make_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color,
		ui_rect &&rect)
		{
			return true;
		}
virtual	bool make_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color,
		ui_rect &&rect)
		{
			return true;
		}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color){}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel &pix){}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel *&pix,
		bool bfree = false)
		{
			if(bfree)
			{
				delete pix;
				pix = nullptr;
			}
		}
virtual	void update_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color){}
virtual void write_user(struct pe_user &&user)
{
	struct platform_par par;
	par.event = platform_event_user;
	par.user = user;
	WQ_send(_workq, &par, sizeof(par), INFINITE, 0);
}
virtual void write_user(struct pe_user &user)
{
	struct platform_par par;
	par.event = platform_event_user;
	par.user = user;
	WQ_send(_workq, &par, sizeof(par), INFINITE, 0);
}


virtual triple_int display_available()
		{
			/*return normally use format */
			return triple_int(320, 240, (int)AV_PIX_FMT_RGB24);
		}
virtual	bool install_event_filter(ui_event_filter &&filter)
		{
			return install_event_filter(filter);
		}
virtual	bool install_event_filter(const ui_event_filter &filter)
{
	_filter = filter;
		return true;
}
virtual bool install_audio_thread(audio_read &&reader,
	int channel, 
	int samplingrate, 
	int samplesize, 
	enum AVSampleFormat fmt)
{
	return true;
}

virtual void uninstall_audio_thread(){}
virtual void run_audio_thread()
{
}
virtual void stop_audio_thread()
{
}

virtual	int exec()
		{
			void * par = NULL;
			_dword size= 0;

			/*
				start our main loop
			*/
			set_loopflag(true);
			while(_bloop_flag)
			{
				_dword res = WQ_recv(_workq, &par, &size, INFINITE);
				if(res != WQ_SIGNALED)
				{
					return -1;
				}
				ui_event e(*(struct platform_par *)par);
				if(_filter)_filter(e);
			}
			return 0;
		}	
};

