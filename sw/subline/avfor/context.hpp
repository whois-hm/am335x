#pragma once
#define section_connection			"[connection]"
#define connection_ip					"connection_ip"
#define connection_port				"connection_port"

#define section_client					"[client]"
#define play_source					"play_source"
#define decode_video_width 			"decode_video_width"
#define decode_video_height 			"decode_video_height"
#define decode_video_format 			"decode_video_format"
#define decode_audio_channel 		"decode_audio_channel"
#define decode_audio_samplingrate 	"decode_audio_samplingrate"
#define decode_audio_format 			"decode_audio_format"
#define authentication_id 			"authentication_id"
#define authentication_password 		"authentication_password"


#define section_server 			"[server]"
#define url 							"url"
#define proxy_autentication_id 			"proxy_autentication_id"
#define proxy_autentication_password 	"proxy_autentication_password"
#define server_session_name 				"server_session_name"
#define server_port 						"server_port"
#define server_autentication_id 			"server_autentication_id"
#define server_autentication_password 	"server_autentication_password"


#define section_gui 			"[gui]"
#define video_width 			"video_width"
#define video_height 			"video_height"
#define video_format 			"video_format"
#define audio_channel 			"audio_channel"
#define audio_samplingrate 	"audio_samplingrate"
#define audio_format 			"audio_format"
#define audio_samplesize 		"audio_samplesize"
#define image_path				"image_path"


class avfor_context
{
private:
	std::list<std::pair<std::string, avattr>> _values;
	bool line_is_comment(std::string &str)
	{
		return str.size( ) > 0 &&
				str.at(0) == '#';
	}
	bool line_is_empty(std::string &str)
	{
		return str.empty();
	}
	bool line_is_jump(std::string &str)
	{
		return line_is_comment(str) ||
				line_is_empty(str);
	}
	bool line_is_section(std::string &str)
	{


		static char const *section_table [] =
		{
				section_connection,
				section_client,
				section_server,
				section_gui
		};

		unsigned ntable = DIM(section_table);
		while(ntable--)
		{
			std::string t(section_table[ntable]);

			if(t == str)
			{
				return true;
			}
		}
		return false;
	}
	int line_is_split(std::string str, std::pair<std::string, std::string> & kv)
	{
		/*
		 	 return == -1 can't find splitor '='
		 	 return == -2 value has empty
		 	 return == -3 key has empty
		 	 return == -4 str is empty
		 	 return == 0 ok
		 */
		if(str.empty())
		{
			return -4;
		}
		int size = str.size();
		std::string::size_type pos = str.find("=");
		if(pos == std::string::npos)
		{
			return -1;
		}
		if(pos == 0)
		{
			return -3;
		}
		if(pos == size - 1)
		{
			return -2;
		}
		kv = std::make_pair(str.substr(0, pos),
				str.substr(pos + 1, std::string::npos));
		return 0;
	}
	int getline(std::ifstream &stream, std::string &line)
	{
		if(stream.eof())
		{
			return -1;
		}
#define atoz(s, a,z) (s>=a&&s<=z)

		char buffer[256] = {0, };
		int len;
		stream.getline(buffer, 255,'\n');
		len = strlen(buffer);
		for(int i = 0; i < len; i++)
		{
			if(buffer[i] == 0x0d)
			{
				buffer[i] = 0;
			}
		}
		line = buffer;

		return 0;
	}


	void find_section(std::ifstream &stream, std::string exist = std::string(""))
	{
		std::string str;
		do
		{
			if(exist.empty())
			{
				if(getline(stream, str) < 0)
				{
					/*
					 	 end parse
					 */
					return ;
				}
			}
			else
			{
				str = exist;
			}
			if(line_is_jump(str))
			{
				break;
			}
			if(!line_is_section(str))
			{
				break;
			}

			_values.push_back(std::make_pair(str, avattr()));
			printf("section find : %s\n", str.c_str());
			find_kv(_values.back().second, stream);
		}while(0);
		return find_section(stream);
	}


	void find_kv( avattr &attr,
			std::ifstream &stream)
	{



		static char const *kv_table[] =
		{
				connection_ip,
				connection_port,

				play_source,
				decode_video_width,
				decode_video_height,
				decode_video_format,
				decode_audio_channel,
				decode_audio_samplingrate,
				decode_audio_format 		,
				authentication_id 			,
				authentication_password	,

				url 					,
				proxy_autentication_id 		,
				proxy_autentication_password,
				server_session_name 		,
				server_port 					,
				server_autentication_id		,
				server_autentication_password,
					

				video_width 	,
				video_height 	,
				video_format 	,
				audio_channel ,
				audio_samplingrate,
				audio_format 		,
				audio_samplesize,
				image_path
		};

		std::string str;
		std::pair<std::string, std::string> p;
		do
		{
			if(getline(stream, str) < 0)
			{
				/*
				 	 end parse
				 */
				return ;
			}
			if(line_is_jump(str))
			{
				break;
			}

			if(line_is_section(str))
			{
				/*
				 	 the new section find
				 */
				return find_section(stream, str);
			}
			if(line_is_split(str, p))
			{
				/*
				 	 unknown ;; next line find
				 */

				break;
			}
			for(int i = 0; i < DIM(kv_table); i++)
			{
				if(p.first == std::string(kv_table[i]))
				{
					attr.set(p.first,
							p.second,
							atoi(p.second.c_str()),	/*just zero set if can't convert */
							atof(p.second.c_str()));/*just zero set if can't convert */
					break;
				}
			}
		}while(0);

		return find_kv( attr,
				stream);
	}
	bool values_check_unique_section()
	{
		int no_duplicate_section = 0;
		if(has_section(section_client)) 			no_duplicate_section++;
		if(has_section(section_server)) 	no_duplicate_section++;
		return has_section(section_connection) &&
				no_duplicate_section == 1;
	}

	bool values_check()
	{
		return values_check_unique_section();
	}

	avattr *ref(char const *section,
			char const *key)
	{
		for(auto &it : _values)
		{
			if(it.first == section)
			{
				if(!it.second.notfound(key))
				{
					return &it.second;
				}
				break;
			}
		}
		return nullptr;
	}
public:
	avfor_context(int argc, char **argv)
	{

		std::ifstream stream;
		stream.open(argv[1], std::ios::in);
		DECLARE_THROW(!stream.is_open(), "can't open setup file");
		find_section(stream);
		stream.close();
		DECLARE_THROW(!values_check(), "invalid setup file");


	}
	bool get(char const *section,
			char const *key,
			std::string &value)
	{
		avattr *attr = ref(section, key);
		if(attr)
		{
			value = attr->get_str(key, "");
			return true;
		}
		return false;
	}
	bool get(char const *section,
			char const *key,
			int &value)
	{
		avattr *attr = ref(section, key);
		if(attr)
		{
			value = attr->get_int(key, 0);
			return true;
		}
		return false;
	}
	bool get(char const *section,
			char const *key,
			double &value)
	{
		avattr *attr = ref(section, key);
		if(attr)
		{
			value = attr->get_double(key, 0.0);
			return true;
		}
		return false;
	}
	bool has_section(char const *section)
	{
		for(auto &it : _values)
		{
			if(it.first == section)
			{
				return true;
			}
		}
		return false;
	}



};



/*
 	 our custom event code
 */
#define custom_code_section_client_end_video				0
#define custom_code_section_client_read_pixel				1
#define custom_code_section_client_ready_to_play			2
#define custom_code_section_client_presentation_tme		4
#define custom_code_util_cpu_usage_notify					3
#define custom_code_util_cpu_usage_stop_notify					5

#define custom_code_section_connection_recv_command		6
#define custom_code_section_connection_close				7

#define custom_code_section_server_incoming_client	8
#define custom_code_section_server_teardown_client	9
#define custom_code_section_server_streamout_reference	10


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
#include "connection_manager.hpp"
#include "client_manager.hpp"
#include "server_manager.hpp"
#include "cpu_manager.hpp"
#include "gpio_manager.hpp"




