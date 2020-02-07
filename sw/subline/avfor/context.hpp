#pragma once


#define pfx_avfor_video_width 			"avfor_video_width"			
#define pfx_avfor_video_height 			"avfor_video_height"
#define pfx_avfor_video_vga 			"avfor_video_vga"
#define pfx_avfor_video_description 	"avfor_video_vga_description"
#define pfx_avfor_runtype 				"avfor_runtype"
#define pfx_avfor_playlist 				"avfor_playlist"
#define pfx_avfor_video_format 			"avfor_video_format"	
#define pfx_avfor_video_format_type 	"avfor_video_format_type"	

#define custom_code_end_video		0
#define custom_code_read_pixel		1
#define custom_code_ready_to_play	2
#define custom_code_cpu_usage_notify	3

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
		printf("-----------------------------------\n");
		printf("---------video format support------\n");
		printf("format : rgb16 (rgb565)\n");
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
	void parse_par(int argc, char *argv[])
	{
		int opt;
		while(-1 != (opt = getopt(argc, argv, "t:l:r:v:")))
		{
			if(opt == 't') set_runtype(optarg);
			else if(opt == 'l') set_playing_list(optarg);
			else if(opt == 'r') set_vga(optarg);
			else if(opt == 'v') set_vformat(optarg);
			else {print_help("invalid format"); exit(0);}
		}
		if(!has_vaild_context())
		{
			printf("can't start avfor (option has invalid)\n");
			exit(0);
		}
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
			{"rgb16", AV_PIX_FMT_RGB565}
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
			!_attr.notfound(pfx_avfor_playlist) &&		/*for output play list*/	
			!_attr.notfound(pfx_avfor_video_format) &&	/*for output video format*/
			!_attr.notfound(pfx_avfor_video_format_type))/*for output video format*/
		{
			return true;
		}
	}
	bool has_audio_output_context()
	{
		return false;
	}
	bool has_vaild_context()
	{
		if(_attr.notfound(pfx_avfor_runtype))
		{
			/*can't fine running type*/
			return false;
		}
		/*
			current support client only
		*/
		if(_attr.get_str(pfx_avfor_runtype) == "client")
		{	
			if(has_video_output_context() || 
				has_audio_output_context())
			{
				return true;
			}
		}
		return false;
	}
	avattr _attr;
};


