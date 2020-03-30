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
			DECLARE_THROW(nvideo_width > std::get<0>(display_fmt) ||
					nvideo_height > std::get<1>(display_fmt) ||
					nvideo_format != std::get<2>(display_fmt),
					"can't open display, parameter has invalid from supported display");

			DECLARE_THROW(!interface->make_window("mainwindow",ui_rect(0,
					0,
					nvideo_width,
					nvideo_height)),
					"can't make window ");

			DECLARE_THROW(!interface->make_pannel("mainwindow",
				"display area",
				ui_color(0, 0, 0),
				ui_rect(0, 0,
						nvideo_width,
						nvideo_height)),
						"can't make pannel");

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
			DECLARE_THROW(!interface->install_audio_thread(nullptr,
					naudio_channel,
					naudio_samplingrate,
					naudio_samplesize,
					(enum AVSampleFormat)naudio_format),
					"can't open audio system") ;

			interface->uninstall_audio_thread();
			has_a = true;
		}
		DECLARE_THROW(!has_v && !has_a, "can't open gui system, your parameter has invalid");
		return interface;
	}

	avfor(int argc, char **argv) :
		_avc(nullptr),
		_int(nullptr)
	{
		livemedia_pp::ref();

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
	void cleanup_par(ui_event &e)
	{
		if(e.what() == platform_event_user)
		{
			if(e.user()->_code == custom_code_section_connection_recv_command)
			{
				free(e.user()->_ptr);
			}
			if(e.user()->_code == custom_code_util_cpu_usage_notify)
			{
				struct cpu_manager_par *par = (struct cpu_manager_par *)e.user()->_ptr;
				delete par;
			}
		}
	}

	/*
	 	 our main loop
	 */
	int operator()()
	{
		int res = 0;
		DECLARE_LIVEMEDIA_NAMEDTHREAD("main");

		/* on the client */
		do
		{
			if(_avc->has_section(section_client))
			{
				load_manager<client_manager>(res);
			}
			if(_avc->has_section(section_server))
			{
				load_manager<server_manager>(res);
			}
			/* no managers load */
			if(!res)
			{
				break;
			}
			/* last manager load */
			load_manager<cpu_manager>(res);
			load_manager<gpio_manager>(res);
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

			printf("avfor loop start\n");
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
			cleanup_par(e);

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

//	livemedia_pp::ref();
//	mediacontainer a("/home/hmkwon/workspace_kp/resource/a.mp4");
//	AVBitStreamFilterContext *filter = av_bitstream_filter_init("h264_mp4toannexb");
//	//a.find_stream(AVMEDIA_TYPE_VIDEO)->codec;
//	while(1)
//	{
//		avpacket_class  pkt;
//		enum AVMediaType current_readtype = AVMEDIA_TYPE_VIDEO;
//		bool res = a.read_packet(current_readtype, pkt);
//		if(!res )
//		{
//			break;
//		}
//
//		if(current_readtype == AVMEDIA_TYPE_VIDEO)
//		{
//			Time_sleep(1000);
//			printf("================VIDEO BUFFER===========\n");
//			printf("prev filter size = %d\n", pkt.raw()->size);
//			Printf_memory(pkt.raw()->data, pkt.raw()->size);
//			av_bitstream_filter_filter(filter, a.find_stream(AVMEDIA_TYPE_VIDEO)->codec, NULL, &pkt.raw()->data, &pkt.raw()->size, pkt.raw()->data, pkt.raw()->size, 0);
//			printf("after filter size = %d\n", pkt.raw()->size);
//			Printf_memory(pkt.raw()->data, pkt.raw()->size);
//		}
//
//
//
//		current_readtype = AVMEDIA_TYPE_VIDEO == current_readtype ?
//				AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;
//	}
};


