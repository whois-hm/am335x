#include "core.hpp"
#include "context.hpp"






class avfor
{
	avfor_context  *_avc;
	ui *_int;
	std::list<manager *> _managers;

	template <typename _T>
	void load_manager(int &res)
	{
		_managers.push_back((manager *)new _T(_avc, _int));
		res++;
	}
public:
	avfor(avfor &&rhs) = delete;
	avfor(const avfor &rhs) = delete;
	avfor(avfor &rhs) = delete;


	avfor(int argc, char **argv) :
		_avc(nullptr),
		_int(nullptr)
	{
		triple_int display_fmt;
		int title_min_h = -1;
		int title_min_w = -1;
		int btn_min_w = -1;
		int btn_min_h = -1;
		int time_min_h = -1;
		int time_min_w = -1;
		int cpu_usage_min_w = -1;
		int cpu_usage_min_h = -1;
		int vwidth = -1;
		int vheight = -1;
		int cpu_vwidth = -1;
		int cpu_vheight = -1;
		enum AVPixelFormat vfmt = AV_PIX_FMT_NONE;
		do
		{
			printf("welcome avfor main\n");
			livemedia_pp::ref();
			throw_register_sys_except();

			_avc = new  avfor_context();
			if(!_avc->parse_par(argc,argv))
			{
				printf("can't parse parameters\n");
				break;
			}
			printf("11\n");
			_int = new ui_platform("user interface");
			printf("1122\n");
			display_fmt = _int->display_available();
			if(std::get<0>(display_fmt) <= 0 &&
					std::get<1>(display_fmt) <= 0 &&
					std::get<2>(display_fmt) < 0)
			{
				printf("can't userinterface load display has not available\n");
				break;
			}
			/*
				draw enable always, if possible use platform display
			*/
			title_min_h = 30;
			title_min_w = 10;
			btn_min_w = 20;
			btn_min_h = 20;
			time_min_h = 10;
			time_min_w = 10;
			cpu_usage_min_w = 10;
			cpu_usage_min_h = 10;
			vwidth = _avc->_attr.get_int(pfx_avfor_video_width,320);
			vheight = _avc->_attr.get_int(pfx_avfor_video_height,240);
			cpu_vwidth = vwidth / 2;
			cpu_vheight = vheight;
			vfmt = (enum AVPixelFormat)_avc->_attr.get_int(pfx_avfor_video_format_type,(int)AV_PIX_FMT_RGB565);
			if(vwidth + cpu_vwidth > std::min(_avc->_attr.get_int(pfx_avfor_frame_width), std::get<0>(display_fmt))||
					vheight + (time_min_h * 5) + title_min_h  + btn_min_h> std::min(_avc->_attr.get_int(pfx_avfor_frame_height), std::get<1>(display_fmt)) ||
					vfmt != std::get<2>(display_fmt))
			{
				printf("can't draw dispay invalid size parameters\n");
				break;
			}

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

			_int->make_window("mainwindow",ui_rect(2000,
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
				printf("111111222\n");
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
				_int->install_event_filter(std::bind(&avfor::handler,
						this,
						std::placeholders::_1));

				return;
		}while(0);
		/*
		 	 can't run
		 */
		exit(0);
	}
	~avfor()
	{
		for(auto &it : _managers)
		{
			delete it;
		}
		_managers.clear();
		if(_int) delete _int;
		if(_avc) delete _avc;
		livemedia_pp::ref(false);
		printf("bye avfor\n");
	}

	/*
	 	 our main loop
	 */
	int operator()()
	{
		int res = 0;

		/* on the client */
		do
		{
			if(_avc->_attr.get_str(pfx_avfor_runtype) == "client")
			{
				load_manager<client_manager>(res);
			}

			/* no managers load */
			if(!res)
			{
				break;
			}
			/* last manager load */
			load_manager<cpu_manager>(res);

			for(auto &it : _managers)
			{
				if(!it->ready()) { res = -1; break; }
			}
			/* loop fail if manager ready fail */
			if(res < 0)
			{
				break;
			}
			/*now startp*/

			res = _int->exec();
		}while(0);

		return res;
	}
	/*
		 	 our main event handler
	*/
	void handler(ui_event &e)
	{

		do
		{
			if(e.what() == platform_event_error)
			{

				break;
			}
			if(e.what() == platform_event_touch)
			{
				if(e.effected_widget_hint("close btn"))
				{
					break;
				}
			}
			unsigned handler_starttime = sys_time_c();
			for(auto &it : _managers)
			{
				/*throw to manager*/
				(*it)(e);
			}
			//printf("main handler return time : %u\n", sys_time_c() - handler_starttime);
			return;
		}while(0);

		_int->set_loopflag(false);
	}
};




/*--------------------------------------
	avfor : main
--------------------------------------*/
int main(int argc, char *argv[])
{
	return avfor(argc, argv)();
};


