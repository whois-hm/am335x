#pragma once
/*
	ui platform on the embedded device, 
	we use display = 'framebuffer'
	           touch(mouse) = 'tslib'

	so for use display, compile with framebuffer dirver env ex) -Dfbenv=\""/dev/fb0\""
	and, for use touch, compile with -Dtsplug and link library 'tslib'

	last notify compile -Dplatform_embedded
*/
/*--------------------------------------------------------------
   reference one point touch driver  	
*/
#if defined (tsplug)	/*if enable ts*/
#include "touch.hpp"
inline touch *touch_ref(bool bcreate = true)
{
	static touch *tou = nullptr;
	if(bcreate)
	{
		if(!tou)
		{
			tou = new touch(true);
		}
		return tou;
	}
	if(tou)
	{
		delete tou;
		tou = nullptr;
	}
}
#else	/*if disable ts*/
inline touch *touch_ref(bool bcreate = true)
{
	return nullptr;
}

#endif
/*--------------------------------------------------------------
   reference one point framebuffer driver  	
*/

#if defined (fbenv)	/*if enable fb*/
#include "directfb.hpp"
inline directfb *fb_ref(
	int x = -1,
	int y = -1,
	int w = -1, 
	int h = -1,
	bool bcreate = true)
{
	static directfb *fb = nullptr;
	if(bcreate)
	{
		if(!fb)
		{
			fb = new directfb(fbenv, x, y, w, h);
		}
		return fb;
	}
	if(fb)
	{
		delete fb;
		fb = nullptr;
	}
	return nullptr;
}
#else/*if disable fb*/
inline direct_fb *fb_ref(
	int x = -1,
	int y = -1,
	int w = -1, 
	int h = -1,
	bool bcreate = true)
{
	return nullptr;
}
#endif


/*
	base define platform embedded environment.
	
	we offer simply output, so not control driver inner class ,
	inner class just mean that symbolical and store for local value
	control only outside class 'ui_platform_embedded'
*/
class ui_platform_embedded : 
	public ui
{
private:
			class widget
			{
			public:
				widget(ui_handle n, 
					const ui_color &color,
					const ui_rect &rect) : 
					_handle(n),
					_color(_color), 
					_rect(rect){}
				virtual ~widget(){}
				ui_handle _handle;
				ui_color _color;
				ui_rect _rect;
			};
			class pannel : public widget
			{
			public:
				pannel(ui_handle n, 
					const ui_color &color,
					const ui_rect &rect) : 
					widget(n, color, rect){}
				virtual ~pannel(){}
			};
			class label : public widget
			{
			public:
				label(ui_handle n, 
					char const *label,
					const ui_color &color,
					const ui_rect &rect) : 
					widget(n, color, rect),
						_label(label){}
				virtual ~label(){}
				ui_handle _handle;
				ui_color _color;
				ui_rect _rect;
				char const *_label; 
			};
			
			
			class window
			{
			public:
				window(ui_handle n, 
					const ui_rect &rect):
					_handle(n), 
					_rect(rect){}
				virtual ~window()
				{
					for(auto &it : _widgets)
					{
						delete it;
					}
					_widgets.clear();
				}
				void widget_push_back(widget *w)
				{
					_widgets.push_back(w);
				}
				widget *find_widget(ui_handle h)
				{
					if(!h)
					{
						return nullptr;
					}
					for(auto &it : _widgets)
					{
						if(!strcmp(it->_handle, h))
						{
							return it;
						}
					}
					return nullptr;					
				}
				ui_handle _handle;
				ui_rect _rect;
				std::list<widget *> _widgets;
			};
private:
	void event_handling_error(ui_event &e)
	{
		
	}
		
	void event_handling_touch(ui_event &e)
	{
		for(auto &it : _windows)
		{
			if(std::get<0>(it->_rect) >= e.touch()->x && 
				(std::get<0>(it->_rect) + std::get<2>(it->_rect)) <= (e.touch()->x) &&
				std::get<1>(it->_rect) >= e.touch()->y && 
				(std::get<1>(it->_rect) + std::get<3>(it->_rect)) <= (e.touch()->y))
			{
				e._effect_windows.push_back(std::string(it->_handle));
			}
			for(auto &wit : it->_widgets)
			{
				int x =  std::get<0>(it->_rect) +  std::get<0>(wit->_rect);
				int y = std::get<1>(it->_rect) +  std::get<1>(wit->_rect);
				int w = x + std::min(std::get<2>(it->_rect), std::get<2>(wit->_rect));
				int h = h + std::min(std::get<3>(it->_rect), std::get<3>(wit->_rect));
				if(x >= e.touch()->x && 
					w <= (e.touch()->x) &&
					y >= e.touch()->y && 
					y <= (e.touch()->y))
				{
					e._effect_widgets.push_back(std::string(it->_handle));
				}				
			}					
		}
				
	}
	void event_filtering(struct platform_par *par)
	{
		if(_filter)
		{		
			ui_event e(*par);

			struct 
			{
				enum platform_event e;
				void (ui_platform_embedded::*fn)(ui_event &e);
			}static filter_table[] = 
			{
				{platform_event_touch, &ui_platform_embedded::event_handling_touch},
					{platform_event_error, &ui_platform_embedded::event_handling_error}
			};

			for(int i = 0; i < DIM(filter_table); i++)
			{
				if(filter_table[i].e == e.what())
				{
					(this->*filter_table[i].fn)(e);
					break;
				}
			}

			_filter(e);
		}		
	}
	window *find_window(ui_handle n)
	{
		if(!n)
		{
			return nullptr;
		}
		for(auto &it : _windows)
		{
			if(!strcmp(it->_handle, n))
			{
				return it;
			}
		}
		return nullptr;
	}
public:
	ui_platform_embedded(char const *name) :
		ui(name, platform_on_embedded),
		_workq(nullptr),
		_filter(nullptr),
		_ts_reader_thread(nullptr)

	{
		_pipe[0] = -1;
		_pipe[1] = -1;

		/*full buffer request*/
		fb_ref();
		touch_ref();
		_platform = platform_on_embedded;		
	}
	virtual ~ui_platform_embedded()
	{
		if(_pipe[1] != -1)
		{
			char a = 1;
			write(_pipe[1], &a, 1);			
		}
		if(_ts_reader_thread)
		{
			_ts_reader_thread->join();
			delete _ts_reader_thread;
			_ts_reader_thread = nullptr;
		}
		if(_pipe[0] != -1) close (_pipe[0]);
		if(_pipe[1] != -1) close (_pipe[0]);
		_pipe[0] = -1;
		_pipe[1] = -1;

		/*
			when application has exit, 
			revert to the color originally created
		*/
		for(auto &it : _windows)
		{
			if(fb_ref())
			{			
 				fb_ref()->draw(triple_int(0,0,0),
				std::get<0>(it->_rect),
				std::get<1>(it->_rect),
				std::get<2>(it->_rect),
				std::get<3>(it->_rect));		
			}

			delete it;
		}		
		_windows.clear();

		WQ_close(&_workq);

		fb_ref(-1, -1, -1, -1, false);
		touch_ref(false);
		
	}
virtual bool make_window(ui_handle win_name, 
		ui_rect &&rect)
	{
		if(fb_ref())
		{
			fb_ref()->draw(triple_int(0,0,0),
				std::get<0>(rect),
			std::get<1>(rect),
			std::get<2>(rect),
			std::get<3>(rect));

			_windows.push_back(new window(win_name, rect));
			return true;
		}
		return false;
	}
virtual bool make_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color,
		ui_rect &&rect)
	{
		window *target = find_window(parent_win);
		if(!target)
		{
			return false;
			
		}
		
		fb_ref()->draw(color,
		std::get<0>(target->_rect) + std::get<0>(rect),
		std::get<1>(target->_rect) + std::get<1>(rect),
		std::min(std::get<2>(target->_rect), std::get<2>(rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(rect)));
		target->widget_push_back(new pannel(pannel_name,
			color,
			rect));
		return true;		
	}
virtual bool make_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color,
		ui_rect &&rect)
	{
		window *target = find_window(parent_win);
		if(!target)
		{
			return false;
			
		}
		
		fb_ref()->draw(text,
		triple_int(std::get<0>(color), std::get<1>(color), std::get<2>(color)),
		std::get<0>(target->_rect) + std::get<0>(rect),
		std::get<1>(target->_rect) + std::get<1>(rect),
		std::min(std::get<2>(target->_rect), std::get<2>(rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(rect)));
		target->widget_push_back(new label(label_name,
			text,
			color,
			rect));
		return true;
	}
virtual void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color)
	{
		window *target = find_window(parent_win);
		if(!target)
		{
			return; 
		}
		widget *wid = target->find_widget(pannel_name);
		if(!wid)
		{
			return;
		}
		wid->_color = color;

		
 		fb_ref()->draw(wid->_color,
		std::get<0>(target->_rect) + std::get<0>(wid->_rect),
		std::get<1>(target->_rect) + std::get<1>(wid->_rect),
		std::min(std::get<2>(target->_rect), std::get<2>(wid->_rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(wid->_rect)));
	}

virtual void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel &pix)
	{
		window *target = find_window(parent_win);
		if(!target)
		{
			return; 
		}
		pannel *wid = (pannel * )target->find_widget(pannel_name);
		if(!wid)
		{
			return;
		}
		
		if(!pix.can_take() ||
			pix.format() != fb_ref()->get_pixfmt() || 
			pix.width() != std::get<2>(wid->_rect) || 
			pix.height() != std::get<3>(wid->_rect))
		{
			return;
		}
		fb_ref()->draw((void *)pix.read(),
				std::get<0>(target->_rect) + std::get<0>(wid->_rect),
		std::get<1>(target->_rect) + std::get<1>(wid->_rect),
		std::min(std::get<2>(target->_rect), std::get<2>(wid->_rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(wid->_rect)));
	}
virtual void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel *&pix,
		bool bfree = false)
	{
		if(pix)
		{
			update_pannel(parent_win, 
				pannel_name,
				*pix);
			if(bfree)
			{
				delete pix;
				pix = nullptr;
			}
		}
	}
virtual void update_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color)
	{
		window *target = find_window(parent_win);
		if(!target)
		{
			return; 
		}
		label *wid = (label *)target->find_widget(label_name);
		if(!wid)
		{
			return;
		}
		wid->_color = color;
		wid->_label = text;
		/*
			label has no background,
			so we initialize color from windows
		*/
		fb_ref()->draw(ui_color(0,0,0),
		std::get<0>(target->_rect) + std::get<0>(wid->_rect),
		std::get<1>(target->_rect) + std::get<1>(wid->_rect),
		std::min(std::get<2>(target->_rect), std::get<2>(wid->_rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(wid->_rect)));

		fb_ref()->draw(wid->_label,
		wid->_color,
		std::get<0>(target->_rect) + std::get<0>(wid->_rect),
		std::get<1>(target->_rect) + std::get<1>(wid->_rect),
		std::min(std::get<2>(target->_rect), std::get<2>(wid->_rect)),
		std::min(std::get<3>(target->_rect), std::get<3>(wid->_rect)));
	}
virtual bool install_event_filter(ui_event_filter &&filter)
	{
		_filter = filter;
		return true;
	}
virtual int exec()
	{
		void * par = NULL;
		_dword size= 0;
		/*
			can't start no have interlocked drivers 			
		*/
		if(!fb_ref() && 
			!touch_ref())
		{
			return -1;
		}
		
		_workq =  WQ_open(sizeof(struct platform_par) , 10);
		if(!_workq)
		{
			return -1;
		}
		/*
			run touch driver on thread
		*/
		if(touch_ref())
		{
			if(pipe(_pipe) < 0)
			{
				return -1;
			}
			_ts_reader_thread = new std::thread([&]()->void {
						
						struct pollfd fds[2];
						
						while(1)
						{
							memset(fds, 0, sizeof(struct pollfd) * 2);
							fds[0].fd = _pipe[0];
							fds[1].fd = touch_ref()->fd();
							fds[0].events = (POLLIN | POLLPRI);
							fds[1].events = (POLLIN | POLLPRI);

							int res = poll(fds, 2, -1);
							if(res == -1 || 
								res == 0)
							{
								struct platform_par par;
								par.event = platform_event_error;
								par.error.error_code = -10;
								WQ_send(_workq, &par, sizeof(par), INFINITE, 0);
								break;
							}

							if(fds[0].revents & (POLLIN | POLLPRI))
							{
								break;
							}
							if(fds[1].revents & (POLLIN | POLLPRI))
							{
								touch_ref()->wait_samp();
								triple_int val = touch_ref()->get_samp();
								struct platform_par par;
								par.event = platform_event_touch;
								par.touch.x = std::get<0>(val);
								par.touch.y = std::get<1>(val);
								par.touch.press = std::get<2>(val);
								
								WQ_send(_workq, &par, sizeof(par), INFINITE, 0);
							}
						}
						
				});
		}
		
		/*
			start our main loop
		*/
		while(1)
		{
			_dword res = WQ_recv(_workq, &par, &size, INFINITE);
			if(res != WQ_SIGNALED)
			{
				return -1;
			}
			
			event_filtering((struct platform_par *)par);
		}
	}

virtual enum AVPixelFormat diplay_format(ui_handle  win)
	{
		window *target = find_window(win);
		if(!target)
		{
			return AV_PIX_FMT_NONE; 
		}
		return fb_ref()->get_pixfmt();
	}

private:
	struct WQ *_workq;
	ui_event_filter _filter;
	std::thread *_ts_reader_thread;
	int _pipe[2];	
	std::list<window *> _windows;
};
