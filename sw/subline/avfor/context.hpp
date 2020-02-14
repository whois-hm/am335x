#pragma once

/*
 	 parameter code
 */
#define pfx_avfor_video_width 			"avfor_video_width"			
#define pfx_avfor_video_height 			"avfor_video_height"
#define pfx_avfor_video_vga 			"avfor_video_vga"
#define pfx_avfor_video_description 	"avfor_video_vga_description"
#define pfx_avfor_runtype 				"avfor_runtype"
#define pfx_avfor_playlist 				"avfor_playlist"
#define pfx_avfor_video_format 			"avfor_video_format"	
#define pfx_avfor_video_format_type 	"avfor_video_format_type"	
#define pfx_avfor_frame_width			"avfor_frame_width"
#define pfx_avfor_frame_height			"avfor_frame_height"
#define pfx_avfor_audio_channel			"avfor_audio_channel"
#define pfx_avfor_audio_samplingrate	"avfor_audio_samplingrate"
#define pfx_avfor_audio_format			"avfor_audio_format"


/*
 	 our custom event code
 */
#define custom_code_end_video		0
#define custom_code_read_pixel		1
#define custom_code_ready_to_play	2
#define custom_code_cpu_usage_notify	3
#define custo_code_presentation_tme		4

class avfor_context
{
public:
	void print_help(char const *what = nullptr)
	{
		if (what) printf("invalid %s\n", what);
		printf("-----------------------------------\n");
		printf("-t : select running type \"server\" or \"client\"\n");
		printf("-l : input streaming contents for type client\n");
		printf("-r : resolution for display if you need\n");
		printf("-v : video format for display if you need\n");
		printf("-w : ui frame width\n");
		printf("-h : ui frame height\n");
		printf("-c : number of audio channel\n");
		printf("-s : sampling rate of audio\n");
		printf("-a : format of audio\n");
		printf("-----------------------------------\n");
		printf("---------video format support------\n");
		printf("format : rgb16 (rgb565)\n");
		printf("format : yuv420p (yv12)\n");
		printf("---------audio format support------\n");
		printf("format : s16\n");
		printf("---------video vga support------\n");
		printf("vga : %d %d %s %s\n", 160, 	120, 	"qqVGA", 	"quarter quarter VGA");
		printf("vga : %d %d %s %s\n", 240, 	160, 	"HqVGA", 	"Half quarter VGA");
		printf("vga : %d %d %s %s\n", 320, 	240, 	"qVGA", 	"quarter VGA");
		printf("vga : %d %d %s %s\n", 400, 	240, 	"WqVGA", 	"Wide quarter VGA");
		printf("vga : %d %d %s %s\n", 480, 	320, 	"HVGA", 	"Half-size VGA");
		printf("vga : %d %d %s %s\n", 640, 	480, 	"VGA", 		"-");
		printf("vga : %d %d %s %s\n", 800, 	480, 	"WVGA", 	"Wide VGA");
		printf("vga : %d %d %s %s\n", 854, 	480, 	"FWVGA", 	"Full Wide VGA");
		printf("vga : %d %d %s %s\n", 800, 	600, 	"SVGA", 	"Super VGA");
		printf("vga : %d %d %s %s\n", 960, 	640, 	"DVGA", 	"Double-size VGA");
		printf("vga : %d %d %s %s\n", 1024, 	600, 	"WSVGA", 	"Wide Super VGA");
		printf("-----------------------------------\n");
	}
	avfor_context()
	{

		livemedia_pp::ref();
	}
	~avfor_context()
	{
		livemedia_pp::ref(false);
	}
	bool parse_par(int argc, char *argv[])
	{
		int opt;
		while(-1 != (opt = getopt(argc, argv, "t:l:r:v:w:h:c:s:a:")))
		{
			if(opt == 't') set_runtype(optarg);
			else if(opt == 'l') set_playing_list(optarg);
			else if(opt == 'r') set_vga(optarg);
			else if(opt == 'v') set_vformat(optarg);
			else if(opt == 'w') set_frame_width(optarg);
			else if(opt == 'h') set_frame_height(optarg);
			else if(opt == 'c') set_audio_channel(optarg);
			else if(opt == 's') set_audio_samplingrate(optarg);
			else if(opt == 'a') set_aformat(optarg);
			else {print_help("invalid format"); exit(0);}
		}
		if(!has_vaild_context())
		{
			return false;
		}
		return true;
	}

	void set_playing_list(char const *list)
	{		
		if(list)
		{
			_attr.set(pfx_avfor_playlist,list, 0, 0.0); 
		}
	}
	void set_runtype(char const *type)
	{
		if(type)
		{
			if(!strcmp(type, "server"))
			{
				_attr.set(pfx_avfor_runtype,"server", 0, 0.0); 			
			}
			if(!strcmp(type, "client"))
			{
				_attr.set(pfx_avfor_runtype,"client", 0, 0.0); 
			}
		}
	}
	 void set_audio_channel(const char *optarg)
	 {
	 _attr.set(pfx_avfor_audio_channel, optarg, atoi(optarg), 0.0);
	 }
	 void set_audio_samplingrate(const char *optarg)
	 {
	 _attr.set(pfx_avfor_audio_samplingrate, optarg, atoi(optarg), 0.0);
	 }
	void set_aformat(const char *optarg)
	{
			/*we can select audio format*/
		struct audio_format
		{
			char const *c_format;
			enum AVSampleFormat fmt;
		}audio_fmt_format[] = 
		{
			{"s16", AV_SAMPLE_FMT_S16}
		};
		if(optarg)
		{
			for(int i = 0; i < DIM(audio_fmt_format); i++)
			{
				if(!strcmp(optarg, audio_fmt_format[i].c_format))
				{
					_attr.set(pfx_avfor_audio_format,audio_fmt_format[i].c_format, (int)audio_fmt_format[i].fmt, 0.0); 
					break;
				}
			}
		}
		
	}
			 
	void set_frame_width(char const *c_val)
	{
		_attr.set(pfx_avfor_frame_width, c_val, atoi(c_val), 0.0);
	}
	void set_frame_height(char const *c_val)
	{
		_attr.set(pfx_avfor_frame_height, c_val, atoi(c_val), 0.0);
	}	
	void set_vga(char const *c_vga)
	{
		/*we can control video size 'vga'*/
		struct video_graphics_array
		{	
			unsigned w;
			unsigned h;
			char const *c_vga;
			char const *c_vga_description;
		}video_array[11] =
		{
			{160, 	120, 	"qqVGA", 	"quarter quarter VGA"},
			{240, 	160, 	"HqVGA", 	"Half quarter VGA"},
			{320, 	240, 	"qVGA", 	"quarter VGA"},
			{400, 	240, 	"WqVGA", 	"Wide quarter VGA"},
			{480, 	320, 	"HVGA", 	"Half-size VGA"},
			{640, 	480, 	"VGA", 		"-"},
			{800, 	480, 	"WVGA", 	"Wide VGA"},
			{854, 	480, 	"FWVGA", 	"Full Wide VGA"},
			{800, 	600, 	"SVGA", 	"Super VGA"},
			{960, 	640, 	"DVGA", 	"Double-size VGA"},
			{1024, 	600, 	"WSVGA", 	"Wide Super VGA"}
		};
		if(c_vga)
		{
			for(int i = 0; i < DIM(video_array); i++)
			{
				if(!strcmp(c_vga, video_array[i].c_vga))
				{
					_attr.set(pfx_avfor_video_width,pfx_avfor_video_width, video_array[i].w, 0.0); 
					_attr.set(pfx_avfor_video_height,pfx_avfor_video_height, video_array[i].h, 0.0); 
					_attr.set(pfx_avfor_video_vga,video_array[i].c_vga, 0, 0.0); 
					_attr.set(pfx_avfor_video_description,video_array[i].c_vga_description, 0, 0.0); 
					break;
				}
			}
		}
	}
	void set_vformat(char const *c_format)
	{
		/*we can select display format*/
		struct video_format
		{
			char const *c_format;
			enum AVPixelFormat fmt;
		}video_fmt_format[] = 
		{
			{"rgb16", AV_PIX_FMT_RGB565},	/*may be raw control*/
				{"yuv420p", AV_PIX_FMT_YUV420P},	/*may be SDL link*/
		};
		if(c_format)
		{
			for(int i = 0; i < DIM(video_fmt_format); i++)
			{
				if(!strcmp(c_format, video_fmt_format[i].c_format))
				{
					_attr.set(pfx_avfor_video_format,video_fmt_format[i].c_format, 0, 0.0); 
					_attr.set(pfx_avfor_video_format_type, pfx_avfor_video_format_type,video_fmt_format[i].fmt, 0.0); 
					break;
				}
			}
		}
	}
	bool has_video_output_context()
	{
		if(!_attr.notfound(pfx_avfor_video_width) &&	/*for output video width*/
			!_attr.notfound(pfx_avfor_video_height) &&	/*for output video height*/
			!_attr.notfound(pfx_avfor_video_vga)&& 		/*for output vga from width-height*/
			!_attr.notfound(pfx_avfor_video_description) &&/*for output vga description*/
			!_attr.notfound(pfx_avfor_video_format) &&	/*for output video format*/
			!_attr.notfound(pfx_avfor_video_format_type))/*for output video format*/
		{
			return true;
		}
		return false;
	}
	bool has_client_play_list()
	{
		if(!_attr.notfound(pfx_avfor_playlist))
		{
			return true;
		}			
		return false;
	}
	bool has_audio_output_context()
	{
		if(!_attr.notfound(pfx_avfor_audio_channel) &&
			!_attr.notfound(pfx_avfor_audio_samplingrate) &&
			!_attr.notfound(pfx_avfor_audio_format))
		{
			return true;
		}
				
		return false;
	}
	bool has_vaild_context()
	{
		if(_attr.notfound(pfx_avfor_runtype) || 
			_attr.notfound(pfx_avfor_frame_width)||
			_attr.notfound(pfx_avfor_frame_height))
		{
			/*can't fine running type*/
			return false;
		}
		/*
			current support client only
		*/
		
		if(_attr.get_str(pfx_avfor_runtype) == "client")
		{	
			if(has_client_play_list() &&
				(	has_video_output_context() || 
					has_audio_output_context()
				)
			  )
			{
				return true;
			}
		}
		return false;
	}
	avattr _attr;
};


class manager
{
protected:
	avfor_context  *_avc;
	ui *_int;
	manager() = delete;
	manager(avfor_context  *avc,
	ui *interface) :
		_avc(avc),
		_int(interface){ }
public:
	virtual ~manager(){ }
	virtual bool ready() = 0;
	/*
	 	 manager's event handler
	 */
	virtual void operator()(ui_event &e) = 0;
};
#include "client_manager.hpp"
#include "cpu_manager.hpp"




