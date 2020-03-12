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
	ui * load_gui_if_set()
	{
		ui *interface = nullptr;
		throw_if ti;
		triple_int display_fmt;
		int nvideo_width = 0;
		int nvideo_height = 0;
		int nvideo_format = 0;
		int naudio_channel = 0;
		int naudio_samplingrate = 0;
		int naudio_format = 0;
		int naudio_samplesize = 0;
		bool has_v = false;
		bool has_a = false;
		if(!_avc->has_section(section_gui))
		{
			return new ui_platform_fake("fake ui");
		}
		interface = new ui_platform("user ui");

		_avc->get(section_gui, video_width, nvideo_width);
		_avc->get(section_gui, video_height, nvideo_height);
		_avc->get(section_gui, video_format, nvideo_format);

		_avc->get(section_gui, audio_channel, naudio_channel);
		_avc->get(section_gui, audio_samplingrate, naudio_samplingrate);
		_avc->get(section_gui, audio_format, naudio_format);
		_avc->get(section_gui, audio_samplesize, naudio_samplesize);

		/*
		 	 using display
		 */

		if(nvideo_width > 0 &&
				nvideo_height > 0 &&
				nvideo_format >= 0)
		{
			display_fmt = interface->display_available();
			/*
			 	 this mean that use section and parameter all vaild
			 	 but system display not supported 'throw!'
			 */
			ti(nvideo_width > std::get<0>(display_fmt) ||
					nvideo_height > std::get<1>(display_fmt) ||
					nvideo_format != std::get<2>(display_fmt),
					"can't open display, parameter has invalid from supported display");

			ti(!interface->make_window("mainwindow",ui_rect(0,
					0,
					nvideo_width,
					nvideo_height)),
					"can't make window ");

			ti(!interface->make_pannel("mainwindow",
				"display area",
				ui_color(0, 0, 0),
				ui_rect(0, 0,
						nvideo_width,
						nvideo_height)));

			has_v = true;
		}
		/*
		 	 using audio
		 */
		if(naudio_channel > 0 &&
				naudio_samplingrate > 0 &&
				naudio_samplesize > 0 &&
				naudio_format >= 0)
		{
			/*open test*/
			ti(!interface->install_audio_thread(nullptr,
					naudio_channel,
					naudio_samplingrate,
					naudio_samplesize,
					(enum AVSampleFormat)naudio_format),
					"can't open audio system") ;

			interface->uninstall_audio_thread();
			has_a = true;
		}
		ti(!has_v && !has_a, "can't open gui system, your parameter has invalid");
		return interface;
	}

	avfor(int argc, char **argv) :
		_avc(nullptr),
		_int(nullptr)
	{
		livemedia_pp::ref();
		throw_register_sys_except();
		_avc = new avfor_context(argc, argv);
		_int = load_gui_if_set();
		_int->install_event_filter(std::bind(&avfor::handler,
				this,
				std::placeholders::_1));
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
			if(_avc->has_section(section_client))
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
			/*
			 	 warnning
			 	 connection_manager event parameter has malloc data.
			 	 this malloc data self free in 'connection_manager'
			 	 therefor the data should be process last eventhandler
			 */
			load_manager<connection_manager>(res);

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

			printf("avfor loop start");
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
			for(auto &it : _managers)
			{
				/*throw to manager*/
				(*it)(e);
			}
			if(e.what() == platform_event_error)
			{
				break;
			}

			if(e.what() == platform_event_user)
			{
				if(e.user()->_code == custom_code_section_connection_close)
				{
					break;
				}
			}
			return;
		}while(0);

		_int->set_loopflag(false);
	}
};



/*--------------------------------------
	avfor : main
--------------------------------------*/
int main(int argc, char *argv[])
{			//rtsp://184.72.239.149/vod/mp4:BigBuckBunny_175k.mov
	//rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov
	return avfor(argc, argv)();
};


