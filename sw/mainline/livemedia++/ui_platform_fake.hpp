
#pragma once
class ui_platform_fake : 
	public ui
{
private:
	semaphore _sem;	
	bool  _run;
public :
	ui_platform_fake(char const *name) : 
		ui(name, platform_on_fake){_run = false;}
	virtual ~ui_platform_fake()
		{
			if(_run)
			{
				SEMA_unlock(&_sem);
				SEMA_close(&_sem);
			}
			_run =false;
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

}
virtual void write_user(struct pe_user &user)
{

}


virtual triple_int display_available()

		{
			/*return normally use format */
			return triple_int(320, 240, (in)AV_PIX_FMT_RGB24)
		}
virtual	bool install_event_filter(ui_event_filter &&filter)
		{
			return true;
		}
virtual bool install_audio_thread(audio_read &&reader,
	int channel, 
	int samplingrate, 
	int samplesize, 
	enum AVSampleFormat fmt)
{
	return false;
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
			if(SEMA_open(&_sem, 0, 1) < 0)
			{
				return -1;
			}
			_run = true;
			SEMA_lock(&_sem, INFINITE);
		}	
};

