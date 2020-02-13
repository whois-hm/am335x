#pragma once
#include "SDL.h"
/*
	complile width 'sdl_ttf_fontpath'
*/
#include "SDL_ttf.h"
/*
	so somple platform.
	just link SDL 
	it support any display, audio, input etc...
	also best speed operation.
*/
class ui_platform_sdl : 
	public ui
{
private:
	
		/*
			control inner 
		*/
			class widget
			{
			public:
				widget(ui_handle n, 
					const ui_color &color,
					const ui_rect &rect,
					SDL_Renderer *target_renderer) : 
					_handle(n),
					_color(_color), 
					_rect(rect),
					_target_renderer(target_renderer){}
				virtual ~widget(){}
				ui_handle _handle;
				ui_color _color;
				ui_rect _rect;
				
				SDL_Renderer *_target_renderer;
			};
			class pannel : public widget
			{
			public:
				pannel(ui_handle n, 
					const ui_color &color,
					const ui_rect &rect,
					SDL_Renderer *target_renderer) : 
					widget(n, color, rect, target_renderer),
						_texture(nullptr)
					{
						reload(SDL_PIXELFORMAT_RGBA32);
						draw_color(color);						
					}
				virtual ~pannel()
				{
					_target_renderer = nullptr;
					if(_texture)
					{
						SDL_DestroyTexture(_texture);
					}
					_texture = nullptr;
				}
				pannel(pannel &&rhs) : 
					widget(rhs._handle, 
						rhs._color, 
						rhs._rect,
						rhs._target_renderer),
						_texture(rhs._texture)						
				{
					rhs._target_renderer = nullptr;
					rhs._texture = nullptr;					
				}
				pannel &operator=(pannel &&rhs)
				{
					_handle = rhs._handle;
					_color = rhs._color;
					_rect = rhs._rect;
					_target_renderer = rhs._target_renderer;
					_texture = rhs._texture;
					rhs._target_renderer = nullptr;
					rhs._texture = nullptr;
				}
				void draw_color(const ui_color &c)
				{
					_color = c;
					unsigned char *pixels;
					int pitch;
					int w = std::get<2>(_rect);
					int h = std::get<3>(_rect);
					reload(SDL_PIXELFORMAT_RGBA32);

					SDL_LockTexture(_texture, nullptr, (void **)&pixels, &pitch);

					for(int y = 0; y < h; y++)
					{
						Uint32 *ptr = (Uint32*)(pixels + (pitch * y));
						for(int x = 0; x < w; x++)
						{
							*ptr |= std::get<0>(_color) << 0;	/*b*/
							*ptr |= std::get<1>(_color) << 8;	/*g*/
							*ptr |= std::get<2>(_color) << 16;	/*r*/
							*ptr |= 0 << 24;	/*a*/
							ptr++;
						}
					}
					
					SDL_UnlockTexture(_texture);
					texture_out();
					printf("draw : %s %d %d %d %d\n", _handle, std::get<0>(_rect) , std::get<1>(_rect), std::get<2>(_rect) , std::get<3>(_rect));
				}
				void draw_pixel( pixel &pix)	
				{
					if(!pix.can_take() || 
						pix.width() != std::get<2>(_rect) || 
						pix.height() != std::get<3>(_rect))	
					{
						/*invalid parameters*/
						return;
					}
					if(pix.format() == AV_PIX_FMT_YUV420P)
					{						
						return draw_pixel_fmt_yv12(pix);
					}
				}
				
			private:
				void texture_out()
				{
					SDL_Rect dr;
					dr.x = std::get<0>(_rect);
					dr.y = std::get<1>(_rect);
					dr.w = std::get<2>(_rect);
					dr.h = std::get<3>(_rect);
					SDL_RenderCopy(_target_renderer, 
						_texture, 
						nullptr, 
						&dr);
					SDL_RenderPresent(_target_renderer);
				}
				void draw_pixel_fmt_yv12(pixel &pix)
				{
					reload(SDL_PIXELFORMAT_YV12);
					int w = std::get<2>(_rect); 
					int h = std::get<3>(_rect);

					int y_size = w * h;
					int uv_size = y_size >> 2;
					int y_start = 0;
					int u_start = y_size;
					int v_start = y_start + u_start + uv_size;
					SDL_Rect sr;
					sr.x = sr.y = 0;
					sr.w = w;
					sr.h = h;

					unsigned char *pixel = (unsigned char *)pix.read();
					unsigned char *yplane = pixel + y_start;
					unsigned char *uplane = pixel + u_start;
					unsigned char *vplane = pixel + v_start;
					SDL_UpdateYUVTexture(_texture,
						nullptr,
						yplane, 
						w, 
						uplane,
						w >>1, 
						vplane,
						w >> 1);

					texture_out();
				}
				void reload(Uint32 sdl_fmt)
				{
					if(!_texture)
					{
						/*at first*/
						_texture = SDL_CreateTexture(_target_renderer, 
							sdl_fmt, 
							SDL_TEXTUREACCESS_STREAMING,
							std::get<2>(_rect),
							std::get<3>(_rect));
						return;
					}
					Uint32 refmt;
					SDL_QueryTexture(_texture, 
						&refmt,
						nullptr,
						nullptr,
						nullptr);
					if(sdl_fmt == refmt)
					{
						/*format duplicated*/
						return;
					}
					/*re create texture*/
					SDL_DestroyTexture(_texture);
					_texture = nullptr;
					reload(sdl_fmt);
				}
				SDL_Texture *_texture;
			};
			class label : public widget
			{

			void cleanup()
			{
				if(_texture)
				{
					SDL_DestroyTexture(_texture);
				}
				if(_surface)
				{
					SDL_FreeSurface(_surface);
				}
				_surface = nullptr;
				_texture = nullptr;
			}
			public:
				void draw_label(const ui_color &uicolor, 
					const char *text)
				{
					int w, h;
					cleanup();
					_color = uicolor;
					_label = text;
						{
							SDL_Rect dr = {
								std::get<0>(_rect),
								std::get<1>(_rect),
								std::get<2>(_rect),
								std::get<3>(_rect)
							};
							SDL_RenderFillRect(_target_renderer, &dr);
						}

					SDL_Color color = {
						std::get<0>(_color),
						std::get<1>(_color),
						std::get<2>(_color)
					};
					_surface = TTF_RenderText_Solid(_target_font,
						_label, 
						color);
					_texture = SDL_CreateTextureFromSurface(_target_renderer, _surface);
					SDL_QueryTexture(_texture, nullptr, nullptr, &w, &h);
					SDL_Rect sr = {
						0,
						0,
						w, 
						h
						};

					SDL_Rect dr = {
						std::get<0>(_rect),
						std::get<1>(_rect),
						/*
							sdl font rendering base not font size but texuture size
						*/
						std::min(std::get<2>(_rect), w),
						std::min(std::get<3>(_rect), h)
					};

					SDL_RenderCopy(_target_renderer, _texture, &sr, &dr);
					SDL_RenderPresent(_target_renderer);
				}
							
				label(ui_handle n, 
					char const *text,
					const ui_color &color,
					const ui_rect &rect,
					SDL_Renderer *target_renderer,
					TTF_Font *target_font) : 
					widget(n, color, rect, target_renderer),
						_label(text),
						_texture(nullptr),
						_surface(nullptr),
						_target_font(target_font)
					{
						draw_label(color, text);
					}
				virtual ~label(){}
				char const *_label; 
				SDL_Texture *_texture;
				SDL_Surface *_surface;
				TTF_Font *_target_font;
			};
			
			
			class window
			{
			public:
				window(ui_handle n, 
					const ui_rect &rect):
					_handle(n), 
					_rect(rect),
					_window(nullptr),
					_render(nullptr)
				{
					_window = SDL_CreateWindow(n,
						std::get<0>(_rect),
						std::get<1>(_rect),
						std::get<2>(_rect),
						std::get<3>(_rect),
						SDL_WINDOW_SHOWN);
					_render = SDL_CreateRenderer(_window, -1, 0);
					SDL_SetRenderDrawColor(_render,
						0, 0, 0, 255);
					SDL_Rect r = {std::get<0>(_rect),
						std::get<1>(_rect),
						std::get<2>(_rect),
						std::get<3>(_rect)};
					SDL_RenderFillRect(_render, &r);
					SDL_RenderPresent(_render);
				}
			    window(window &&rhs)
			    {
					_handle = rhs._handle;
					_rect = rhs._rect;
					_window = rhs._window;
					_render = rhs._render;
					_widgets = rhs._widgets;
					rhs._handle = nullptr;
					rhs._window = nullptr;
					rhs._render = nullptr;
					/*pointer copy*/
					//for(auto &it : rhs._widgets)
					//{
						//delete it;
					//}
					rhs._widgets.clear();
			    }
				window &operator=(window &&rhs)
				{
					_handle = rhs._handle;
					_rect = rhs._rect;
					_window = rhs._window;
					_render = rhs._render;
					_widgets = rhs._widgets;
					rhs._handle = nullptr;
					rhs._window = nullptr;
					rhs._render = nullptr;
						/*pointer copy*/
					//for(auto &it : rhs._widgets)
					//{
						//delete it;
					//}
					rhs._widgets.clear();
				}
				virtual ~window()
				{
					for(auto &it : _widgets)
					{
						delete it;
					}
					_widgets.clear();
					if(_window) SDL_DestroyWindow(_window);
					if(_render )SDL_DestroyRenderer(_render);
					_render = nullptr;
					_window = nullptr;
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
				SDL_Window *_window;
				SDL_Renderer *_render;
				std::list<widget *> _widgets;
			};

	
	
public :
	ui_platform_sdl(char const *name) : 
		ui(name, platform_on_sdl),
			_reg_eventid(0),
			_font(nullptr),
			_filter(nullptr)
		{
			SDL_Init(SDL_INIT_EVERYTHING);
			TTF_Init();
			_font = TTF_OpenFont(sdl_ttf_fontpath, 15);
			TTF_SetFontKerning(_font, 1);
			_reg_eventid = SDL_RegisterEvents(1);
		}
	virtual ~ui_platform_sdl()
		{
			uninstall_audio_thread();
			for(auto &it : _windows)
			{
				delete it;
			}		
			_windows.clear();
			TTF_CloseFont(_font);
			TTF_Quit();
			SDL_Quit();			
		}

virtual	bool make_window(ui_handle win_name, 
		ui_rect &&rect)
		{
			/*1 window operation only*/
			if(_windows.empty())
			{
				_windows.push_back(new window(win_name, rect));
				return true;
			}
			return false;
		}
		
virtual	bool make_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color,
		ui_rect &&rect)
		{
			
			window *target = find_window(parent_win);
			if(!target)
			{
				return false;			
			}

			target->widget_push_back(new pannel(pannel_name,
			color,
			rect,
			target->_render));		
			return true;
		}
virtual	bool make_label(ui_handle parent_win,
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
			target->widget_push_back(new label(label_name,
			text,
			color,
			rect,
			target->_render,
			_font));		
			return true;
		}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		ui_color &&color)
		{
			window *target = find_window(parent_win);
			if(!target)
			{
				return ;			
			}
			pannel *wid = (pannel *)target->find_widget(pannel_name);
			if(!wid)
			{
				return;
			}
			wid->draw_color(color);
		}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel &pix)
		{
			window *target = find_window(parent_win);
			if(!target)
			{
				return ;			
			}
			pannel *wid = (pannel *)target->find_widget(pannel_name);
			if(!wid)
			{
				return;
			}
			wid->draw_pixel(pix);
		}
virtual	void update_pannel(ui_handle  parent_win, 
		ui_handle pannel_name,
		pixel *&pix,
		bool bfree = false)
		{
			update_pannel(parent_win, pannel_name, *pix);
			if(bfree)
			{
				delete pix;
				pix = nullptr;
			}
		}
virtual	void update_label(ui_handle parent_win,
		ui_handle label_name,
		char const *text,
		ui_color &&color)
		{
			window *target = find_window(parent_win);
			if(!target)
			{
				return ;			
			}
			label *wid = (label *)target->find_widget(label_name);
			if(!wid)
			{
				return;
			}
			wid->draw_label(color,  text);			
		}
virtual void write_user(struct pe_user &&user)
{
	SDL_Event e;
	memset(&e, 0, sizeof(SDL_Event));
	e.type = _reg_eventid;
	e.user.code = user._code;
	e.user.data1 = (void *)user._ptr;
	SDL_PushEvent(&e);
}
virtual void write_user(struct pe_user &user)
{
	SDL_Event e;
	memset(&e, 0, sizeof(SDL_Event));
	e.type = _reg_eventid;
	e.user.code = user._code;
	e.user.data1 = (void *)user._ptr;
	SDL_PushEvent(&e);
}


virtual triple_int display_available()
		{
			SDL_DisplayMode dm;
			SDL_GetDesktopDisplayMode(0, &dm);
			return triple_int(dm.w, dm.h, (int)AV_PIX_FMT_YUV420P);
		}
virtual	bool install_event_filter(ui_event_filter &&filter)
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
	SDL_AudioFormat sdlformat = 0;
	if(AV_SAMPLE_FMT_S16 == fmt)
	{
		sdlformat = AUDIO_S16SYS;
	}
	if(sdlformat == 0)
	{
		return false;
	}

	uninstall_audio_thread();	

	_auio_read = reader;
	SDL_AudioSpec format;
	format.channels = channel;
	format.freq = samplingrate;
	format.samples = samplesize;
	format.format = sdlformat;
	format.userdata = (void *)this;
	format.callback = ui_platform_sdl::audio_from_call;
	SDL_AudioSpec s;	
	int res = SDL_OpenAudio(&format, &s);
	stop_audio_thread();
	return res;

}
virtual void uninstall_audio_thread()
{
	stop_audio_thread();

	if(_auio_read)
	{
		SDL_CloseAudio();
		_auio_read = nullptr;
	}
}
virtual void run_audio_thread()
{
	if(_auio_read)
	{
		SDL_PauseAudio(0);
	}
}
virtual void stop_audio_thread()
{
	if(_auio_read)
	{
		SDL_PauseAudio(1);
	}
}

virtual	int exec()
		{

			int res = -1;
			set_loopflag(true);
			SDL_Event e;
			while(_bloop_flag)
			{
				res = SDL_WaitEvent(&e);
				if (res > 0)
				{
					event_filtering(&e);
				}
				else
				{
					break;
				}
			}
	
			return res;
		}	
private:
	static void audio_from_call(void *puserdata, 
		unsigned char *pcm, 
		int len)
	{
		ui_platform_sdl *thiz = (ui_platform_sdl *)puserdata;

		thiz->_auio_read(pcm, len);
		
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
		
	void event_handling_user(SDL_Event *e)
	{
		struct platform_par parameter;
		memset(&parameter, 0, sizeof(struct platform_par));
		
		parameter.event = platform_event_user;
		parameter.user._code = e->user.code;
		parameter.user._ptr = e->user.data1;			
		if(_filter)
		{			
			ui_event e(parameter);
			_filter(e);
		}				
	}
	void event_handling_error(SDL_Event *e)
	{
		struct platform_par parameter;
		memset(&parameter, 0, sizeof(struct platform_par));

		parameter.event = platform_event_error;
		parameter.error.error_code = -11;
		if(_filter)
		{			
			ui_event e(parameter);
			_filter(e);
		}
	}
	void event_handling_window(SDL_Event *e)
	{
		struct platform_par parameter;
		memset(&parameter, 0, sizeof(struct platform_par));

		
		/*our main loop return ) no push to user */
		if (e->window.event == SDL_WINDOWEVENT_CLOSE)
		{
			set_loopflag(false);
		}			
	}
	void event_handling_touch(SDL_Event *e)
	{
		struct platform_par parameter;
		memset(&parameter, 0, sizeof(struct platform_par));
		
		parameter.event = platform_event_touch;
		parameter.touch.x = e->button.x;
		parameter.touch.y = e->button.y;
		parameter.touch.press = e->type == SDL_MOUSEBUTTONDOWN ? 125 : 0;

		if(_filter)
		{			
			window *win = _windows.front();
			ui_event ue(parameter);
			ue._effect_windows.push_back(std::string(win->_handle));

			for(auto &wit : win->_widgets)
			{
				if(std::get<0>(wit->_rect) <= ue.touch()->x && 
					std::get<1>(wit->_rect) <= ue.touch()->y &&
					std::get<0>(wit->_rect) + std::get<2>(wit->_rect) >= ue.touch()->x &&
					std::get<1>(wit->_rect) + std::get<3>(wit->_rect) >= ue.touch()->y)
				{
					printf("get %s\n", wit->_handle);
					ue._effect_widgets.push_back(std::string(wit->_handle));
				}
			}
			
			_filter(ue);
		}			
	}	
	void event_filtering(SDL_Event *e)
	{

		if(e->type == _reg_eventid)
		{
			event_handling_user(e);
		}
		else if(e->type == SDL_WINDOWEVENT)
		{
			event_handling_window(e);
		}
		else if(e->type == SDL_MOUSEBUTTONDOWN || 
			e->type == SDL_MOUSEBUTTONUP)
		{
			event_handling_touch(e);
		}
		else
		{
			//event_handling_error(e);
		}
		
	}
private:
	Uint32 _reg_eventid;
	TTF_Font * _font;
	ui_event_filter _filter;
	audio_read _auio_read;
	std::list<window *> _windows;
};

