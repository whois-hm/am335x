#pragma once
/*
	our supported user interface platform
*/
enum platform{
platform_on_embedded, 	/*raw level control platform */
platform_on_sdl,		/*sdl lib support*/
platform_on_fake		/*usefull for no detect platform, because external code is maintain*/
};

typedef char const *ui_handle;		/*window, widgets handle*/
typedef triple_int	ui_color;		/*color rgb*/
typedef std::tuple<int, int, int, int> ui_rect;	/*geometry value*/

/*
	platform's internal notify envent
*/
enum platform_event {
	platform_event_error = 0, 	/*system has error, we recommend go to shutdown */
	platform_event_touch,		/*screen touch has signaled*/
	platform_event_user = 125	/*user custom event*/
};
/*platform_event_touch's parameter*/
struct pe_touch
{
	/*
		x : resolusion x
		y : resolusion y
		press : x > 0 mean finger press  other release
		
	*/
	int x, y, press;
};
/*platform_event_error's parameter*/
struct pe_error
{
	/*
		any platform can't running notify 
		code : -10 -> pipe, or any file description has broken
		code : -11 -> unknown or no support
	*/
	int error_code;
};
struct pe_user
{
	/*
		user custom parameter
	*/

	unsigned _code;
	void *_ptr;
	
};

/*packing platform events*/
struct platform_par
{
	enum platform_event event;
	union
	{
		struct pe_touch touch;
		struct pe_error error;
		struct pe_user user;
	};
};	

/*
	platform events + user notify 
*/
class ui_event
{
public:
	ui_event(struct platform_par parameter) : 
		_par(parameter){}
	ui_event(struct platform_par &&parameter):
		_par(parameter){}
	virtual ~ui_event(){}
	/*get associated widget*/
	bool effected_widget_hint(ui_handle h)
	{
		for(auto &it : _effect_widgets)
		{
			if(it == h)
			{
				return true;
			}
		}
		return false;
	}
	/*get associated windodw*/
	bool effected_window_hint(ui_handle h)
	{
		for(auto &it : _effect_windows)
		{
			if(it == h)
			{
				return true;
			}
		}
		return false;
	}
	enum platform_event what()	/*get event type*/
	{
		return _par.event;
	}
	const struct pe_touch *touch(){return &_par.touch;}		
	const struct pe_error *error(){return &_par.error;}
	const struct pe_user *user(){return &_par.user;}
	std::list<std::string> _effect_windows;
	std::list<std::string> _effect_widgets;
	platform_par _par;
};
typedef std::function<void (ui_event &)> ui_event_filter;
typedef std::function<void (
	unsigned char *, /*pcm*/
	int)/*pcm length*/
	> audio_read;

class ui
{

/*
	warnning note : 

	use for video apis : display api use must be 'main thread'(using 'write_user'),

	use for audio api : we offer exclusive thread for speed
*/
protected:
	enum platform _platform;
	bool _bloop_flag;
public:
		ui(char const *name, 
		enum platform platform = platform_on_fake):
		_platform(platform),
			_bloop_flag(false){}
	virtual ~ui(){}
	void set_loopflag(bool bloop)
	{
	 	_bloop_flag = bloop;
	}
virtual	bool make_window(ui_handle win_name, 
		ui_rect &&rect) = 0;
virtual	bool make_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color,
		ui_rect &&rect) = 0;
virtual	bool make_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color,
		ui_rect &&rect) = 0;
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color) = 0;
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel &pix) = 0;
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel *&pix,
		bool bfree = false) = 0;
virtual	void update_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color) = 0;
/*throw your custom event*/
virtual void write_user(struct pe_user &&user) = 0;
virtual void write_user(struct pe_user &user) = 0;




		/*
			register if you want receiving internal notify event 
		*/
virtual	bool install_event_filter(ui_event_filter &&filter) = 0;
virtual	bool install_event_filter(const ui_event_filter &filter) = 0;
virtual bool install_audio_thread(audio_read &&reader,
	int channel, 
	int samplingrate, 
	int samplesize, 
	enum AVSampleFormat fmt) = 0;		
virtual void uninstall_audio_thread() = 0;
virtual void run_audio_thread() = 0;
virtual void stop_audio_thread() = 0;
		/*
			starting platform
		*/
	virtual	int exec() = 0;
	virtual triple_int display_available() = 0;
	enum platform platform() const 
	{
		return _platform;
	}
	
};
/*
	we select platform base ui

	auto detect from compile time 	
*/
#if defined (platform_embedded)
#include "ui_platform_embedded.hpp"
typedef ui_platform_embedded ui_platform;
#elif defined (platform_sdl)
#include "ui_platform_sdl.hpp"
typedef ui_platform_sdl ui_platform;
#else 

//typedef ui_platform_fake ui_platform;
#endif
#include "ui_platform_fake.hpp"
