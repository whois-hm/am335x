#pragma once
/*
	our supported user interface platform
*/
enum platform{
platform_on_embedded, 	/*raw level control platform */
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
	platform_event_touch		/*screen touch has signaled*/
};
/*platform_event_touch's parameter*/
struct pe_touch
{
	/*
		x : resolusion x
		y : resolusion y
		press : 0 mean finger press  other release
		
	*/
	int x, y, press;
};
/*platform_event_error's parameter*/
struct pe_error
{
	/*
		any platform can't running notify 
		code : -10 -> pipe, or any file description has broken
	*/
	int error_code;
};
/*packing platform events*/
struct platform_par
{
	enum platform_event event;
	union
	{
		struct pe_touch touch;
		struct pe_error error;
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
	std::list<std::string> _effect_windows;
	std::list<std::string> _effect_widgets;
	platform_par _par;
};
typedef std::function<void (ui_event &)> ui_event_filter;

class ui
{
protected:
	ui(char const *name, 
		enum platform platform = platform_on_fake):
		_platform(platform){}
	virtual ~ui(){}

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

virtual enum AVPixelFormat diplay_format(ui_handle  win) = 0;

		/*
			register if you want receiving internal notify event 
		*/
virtual	bool install_event_filter(ui_event_filter &&filter) = 0;
		/*
			starting platform
		*/
virtual	int exec() = 0;
	
	enum platform _platform;
public:
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
#else 
#include "ui_platform_fake.hpp"
typedef ui_platform_fake ui_platform;
#endif
