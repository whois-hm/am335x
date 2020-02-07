#pragma once

class user_interface
{
private:

	display_engine *_display;
	display_engine *create_display(const avattr &attr)
	{
		if(!attr.has_frame_video())
		{
			return nullptr;
		}

		/*
			select display
		*/
		/* select display on linux framebuffer*/
#if defined (with_linux_framebuffer)
		#include "directfb.hpp"
		return new directfb("/dev/fb0", attr.get_int(avattr_key::video_x), 
					attr.get_int(avattr_key::video_y), 
					attr.get_int(avattr_key::width), 
					attr.get_int(avattr_key::height))
#else
		return nullptr;
#endif
	}
	unsigned char install_engines()
	{
		unsigned char code = 0;
		struct
		{
			interface_engine *e;
		}engine_table[] = 
		{
			dynamic_cast<interface_engine *>(_display)
		};
		for(int i = 0; i < DIM(engine_table); i++)
		{
			unsigned char c = 0;
			c = e[i]->install();
			if(!interface_engine::install_code_conf(c, result_code))
			{
				/*engine install fail*/
				return 0;
			}
			if()
			code |= c;
		}
		return code;
	}
public:
	user_interface(const avattr &attr) :
		_our_workqueue(nullptr),
		_display(create_display(attr))			
	{
	
	}		
	void draw(void *data_ptr, 
		int x, 
		int y, 
		int w,
		int h)
	{
		if(_display)
		{
			_display->draw(data_ptr,
				x,
				y,
				w,
				h)
		}
	}
	void draw(const triple_int &rgb,
		int x,
		int y,
		int w, 
		int h)
	{
		if(_display)
		{
			_display->draw(rgb,
				x,
				y,
				w,
				h)
		}
	}
	void draw(char const *str, 
		const triple_int &rgb,
		int x,
		int y)
	{
		if(_display)
		{
			_display->draw(rgb,
				x,
				y)
		}
	}
	int get_start_x()
	{
		if(_display)
		{
			return _display->get_start_x()
		}
		return -1;
	}
	int get_start_y()
	{
		if(_display)
		{
			return _display->get_start_y()
		}
		return -1;
	}
	int get_resolution_x()
	{
		if(_display)
		{
			return _display->get_resolution_x()
		}
		return -1;
	}
	int get_resolution_y()
	{
		if(_display)
		{
			return _display->get_resolution_y()
		}
		return -1;
	}
	enum AVPixelFormat get_pixfmt()
	{
		if(_display)
		{
			return _display->get_resolution_y()
		}
		return AV_PIX_FMT_NONE;
	}
	char const *get_display_device_name()
	{
		if(_display)
		{
			return _display->get_device_name();
		}
		return nullptr;
	}
	struct ui_event waitfor()
	{
		struct ui_event event;
		memset(event, 0, sizeof(struct ui_event));
		
		unsigned char code = install_engines();
		if(code == 0)
		{
			/*one of engine install fail*/
			event._type = ui_event_type_input_broken;
			return event;
		}
		if(interface_engine::install_code_conf(code, need_workqueue_code) &&
			interface_engine::install_code_conf(code, extenal_workqueue_code) )
		{
			/*duplicated workqueue*/
			event._type = ui_event_type_input_broken;
			return event;
		}
		
	}
};


