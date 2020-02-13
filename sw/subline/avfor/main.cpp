#include "core.hpp"
#include "context.hpp"



avfor_context  *_avc = nullptr;
ui *_int = nullptr;
client_manager *_cmanager = nullptr;
cpu_manager *_cpumanager = nullptr;




void handler_touch(ui_event &e)
{
	if(!e.touch()->press)
	{
		if(e.effected_widget_hint("close btn"))
		{
			_int->set_loopflag(false);
		}
		else if(e.effected_widget_hint("left btn"))
		{
			if(_cmanager) _cmanager->seek(-10.0);
		}
		else if(e.effected_widget_hint("right btn"))
		{
			if(_cmanager) _cmanager->seek(10.0);
		}
		else if(e.effected_widget_hint("play btn"))
		{
			if(_cmanager)
			{
				if(_cmanager->get_pause())
				{
					_cmanager->pause(false);
				}
				else
				{
					if(_cmanager) _cmanager->pause(true);
				}
			}
		}

		else if(e.effected_widget_hint("cpu btn"))
		{
			if(!_cpumanager->state())
			{
				_cpumanager->start_cpu_usage(1000);
			}
			else
			{
				_cpumanager->stop_cpu_usage();
				_int->update_label("mainwindow","cpu btn label","cpu show",ui_color(255,0,255));
				_int->update_label("mainwindow","cpu cpu","cpu:-%",ui_color(255,255,0));				
				_int->update_label("mainwindow","cpu sys","cpusys:-%",ui_color(0,255,0));
				_int->update_label("mainwindow","cpu usr","cpuusr:-%",ui_color(0,0,255));
				_int->update_label("mainwindow","cpu idle","cpuidle:-%",ui_color(255,0,255));	
	
			}
		}
	}

}
void handler_user(ui_event &e)
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
		pixel *pix = (pixel *)(e.user()->_ptr);
		_int->update_pannel("mainwindow","display area",pix, true);
	}
	if(e.user()->_code == custo_code_presentation_tme)
	{
		double *v = (double *)(e.user()->_ptr);
		
		if(v)
		{
			double pts = *v;
			if(_cmanager)
			{
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
			}
			free(v);
		}
	}
	if(e.user()->_code == custom_code_ready_to_play)
	{
		duration_div *v = (duration_div *)(e.user()->_ptr);
		if(v)
		{
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
	if(e.user()->_code == custom_code_cpu_usage_notify)
	{
		struct cpu_manager_par *par = (struct cpu_manager_par *)e.user()->_ptr;
		
		char buf[256] = {0, };
		sprintf(buf, "cpu:%f%", par->_cpu);
		_int->update_label("mainwindow","cpu cpu",buf,ui_color(255,255,0));				
		memset(buf, 0, 256);
		sprintf(buf, "cpusys:%f%", par->_sys);
		_int->update_label("mainwindow","cpu sys",buf,ui_color(0,255,0));
		memset(buf, 0, 256);
		sprintf(buf, "cpuusr:%f%", par->_usr);
		_int->update_label("mainwindow","cpu usr",buf,ui_color(0,0,255));
		memset(buf, 0, 256);
		sprintf(buf, "cpuidle:%f%", par->_idle);
		_int->update_label("mainwindow","cpu idle",buf,ui_color(255,0,255));	
	}
}
void handler_error(ui_event &e)
{
	_int->set_loopflag(false);
}


void event_filter_avfor(ui_event &e)
{

	if(e.what() == platform_event_touch)
		handler_touch(e);
		else if(e.what() == platform_event_user)
			handler_user(e);
			else if(e.what() == platform_event_error)
				handler_error(e);
}

/*--------------------------------------
	make interface
--------------------------------------*/
void ready_avfor(int argc, char *argv[])
{			
	_avc = new  avfor_context();
	_avc->parse_par(argc,argv);
	_int = new ui_platform("user interface");
	
	triple_int display_fmt;
	display_fmt = _int->display_available();
	if(std::get<0>(display_fmt) > 0 &&
		std::get<1>(display_fmt) > 0 &&
		std::get<2>(display_fmt) >= 0)
	{
		/*
			draw enable always, if possible use platform display 
		*/
		int title_min_h = 30;
		int title_min_w = 10;
		int btn_min_w = 20;
		int btn_min_h = 20;
		int time_min_h = 10;
		int time_min_w = 10;
		int cpu_usage_min_w = 10;
		int cpu_usage_min_h = 10;
		int vwidth = _avc->_attr.get_int(pfx_avfor_video_width,320);
		int vheight = _avc->_attr.get_int(pfx_avfor_video_height,240);
		int cpu_vwidth = vwidth / 2;
		int cpu_vheight = vheight;

		enum AVPixelFormat vfmt = (enum AVPixelFormat)_avc->_attr.get_int(pfx_avfor_video_format_type,(int)AV_PIX_FMT_RGB565);
		

		if(vwidth + cpu_vwidth > std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt))|| 
				vheight + (time_min_h * 5) + title_min_h  + btn_min_h> std::min(_avc->_attr.get_int(pfx_avfor_frame_height), std::get<1>(display_fmt)) || 
				vfmt != std::get<2>(display_fmt))
		{
			printf("make platform display fail (invalid setup value)\n");
		}
		else
		{
			title_min_h = 30;
			time_min_h 	= (std::min(_avc->_attr.get_int(pfx_avfor_frame_height), std::get<1>(display_fmt)) - title_min_h - (vheight) - (btn_min_h)) / 5;/*pts / duration / cpu / syscpu/usrcpu*/
			btn_min_h 	= std::min(_avc->_attr.get_int(pfx_avfor_frame_height), std::get<1>(display_fmt)) - title_min_h  - vheight - (time_min_h * 5);
			cpu_vheight = vheight;
			cpu_usage_min_h = time_min_h;

			title_min_w = std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt));
			btn_min_w = std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt)) / 5;/*left play right cpu close*/
			time_min_w = vwidth;
			cpu_usage_min_w = std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt)) - vwidth; 
			cpu_vwidth = std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt)) - vwidth; 

			

			_int->make_window("mainwindow",ui_rect(0, 
				0, 
				320, 
				240));
			
			/*draw top*/
			_int->make_label("mainwindow",
				"title",
				"play:pause",
				ui_color(255,255,255),
				ui_rect(0, 0, title_min_w, title_min_h));
						
			/*draw center*/
			_int->make_pannel("mainwindow",
				"display area",
				ui_color(100, 100, 100), 
				ui_rect(0, title_min_h, vwidth, vheight));
			_int->make_pannel("mainwindow",
				"cpu display area",
				ui_color(200, 200, 200), 
				ui_rect(vwidth, title_min_h, cpu_vwidth, cpu_vheight));
			/*draw bottom*/
			_int->make_label("mainwindow",
				"time lable pts",
				"pts:00.00.00",
				ui_color(255,255,0),
				ui_rect(0, vheight + title_min_h , time_min_w, time_min_h));
			_int->make_label("mainwindow",
				"time lable duration",
				"duration:00.00.00",
				ui_color(0,255,255),
				ui_rect(0, vheight + time_min_h + title_min_h, time_min_w, time_min_h));


			_int->make_label("mainwindow",
				"cpu cpu",
				"cpu:-%",
				ui_color(255,255,0),
				ui_rect(vwidth, cpu_vheight + title_min_h , cpu_usage_min_w, cpu_usage_min_h));

			_int->make_label("mainwindow",
				"cpu sys",
				"cpusys:-%",
				ui_color(0,255,0),
				ui_rect(vwidth, cpu_vheight + title_min_h + cpu_usage_min_h , cpu_usage_min_w, cpu_usage_min_h));

			_int->make_label("mainwindow",
				"cpu usr",
				"cpuusr:-%",
				ui_color(0,0,255),
				ui_rect(vwidth, cpu_vheight + title_min_h + cpu_usage_min_h + cpu_usage_min_h, cpu_usage_min_w, cpu_usage_min_h));
			_int->make_label("mainwindow",
				"cpu idle",
				"cpuidle:-%",
				ui_color(255,0,255),
				ui_rect(vwidth, cpu_vheight + title_min_h + cpu_usage_min_h + cpu_usage_min_h + cpu_usage_min_h, cpu_usage_min_w, cpu_usage_min_h));
			/*
				buttons
			*/
			_int->make_pannel("mainwindow",
				"left btn",
				ui_color(255, 0, 0), 
				ui_rect(btn_min_w * 0, title_min_h + time_min_h *5 + vheight, btn_min_w, btn_min_h));
	
			_int->make_pannel("mainwindow",
				"play btn",
				ui_color(255, 255, 0), 
				ui_rect(btn_min_w * 1 , title_min_h + time_min_h *5 + vheight, btn_min_w, btn_min_h));

			_int->make_pannel("mainwindow",
				"right btn",
				ui_color(0, 0, 255), 
				ui_rect(btn_min_w * 2 , title_min_h + time_min_h *5 + vheight, btn_min_w, btn_min_h));
			_int->make_pannel("mainwindow",
				"cpu btn",
				ui_color(255, 0, 255), 
				ui_rect(btn_min_w * 3 , title_min_h + time_min_h *5 + vheight, btn_min_w, btn_min_h));
			_int->make_pannel("mainwindow",
				"close btn",
				ui_color(100, 100, 255), 
				ui_rect(btn_min_w * 4 , title_min_h + time_min_h *5 + vheight, btn_min_w, btn_min_h));
						

			_int->install_event_filter(event_filter_avfor);
		}
	}
	else
	{
		printf("make platform display fail (platform not support)\n");		
	}
}
int loop_avfor()
{
	int res = -1;
	_cpumanager = new cpu_manager();
	/* on the client */
	if(_avc->_attr.get_str(pfx_avfor_runtype) == "client")
	{
		_cmanager = new client_manager ();
		if(_cmanager->can())
		{
			_cmanager->ready_to_play();				
			res = _int->exec();
		}			
	}
	return res;
}
void clean_avfor()
{
	if(_cmanager) delete _cmanager;
	if(_cpumanager) delete _cpumanager;
	if(_int) delete _int;
	if(_avc) delete _avc;	
}



/*--------------------------------------
	avfor : main
--------------------------------------*/
int main(int argc, char *argv[])
{
	int res = -1;
//	printf("welcome avfor main\n");
	ready_avfor(argc, argv);
	res = loop_avfor();
	clean_avfor();
	//printf("bye avfor\n");
	return res;
};


