#pragma once


/*
 	 defined string's

 	 project "livemedia++" class's use this some regulated rules string
 	 like about pixel or video class use key "frame_video", and
 	 like about pcm or audio class use key "frame_audio".

 	 //you need how to use initial value stored class 'avattr'
 */
namespace avattr_key
{
	static constexpr char const * const not_found = "not found";
	static constexpr char const * const frame_video = "frame video";
	static constexpr char const * const frame_audio = "frame audio";
	static constexpr char const * const video_x = "video x";
	static constexpr char const * const video_y = "video y";
	static constexpr char const * const width = "width";
	static constexpr char const * const height = "height";
	static constexpr char const * const pixel_format = "pixel format";
	static constexpr char const * const channel = "channel";
	static constexpr char const * const samplerate = "samplerate";
	static constexpr char const * const pcm_format= "pcm format";

	static constexpr char const * const video_encoderid= "video encoder id";
	static constexpr char const * const audio_encoderid= "audio encoder id";
	static constexpr char const * const fps= "fps";
	static constexpr char const * const bitrate= "bitrate";
	static constexpr char const * const gop= "gop";
	static constexpr char const * const max_bframe= "max bframe";
}


/*
 	 map -> <key> : <value_int, value_double, value_string>
 	 	 	 	 	 <key> : <value_int, value_double, value_string>
 	 	 	 	 	 ................
 	 	 	 	 	 .................

 	 select three value int or double or string.
 	 non use or initialize value is your choice
 */
class avattr
{
public:
	typedef std::string avattr_type_string ;		/*key or value string*/
	typedef int                avattr_type_int;			/*value int*/
	typedef double      avattr_type_double;	/*value double*/
	typedef std::tuple <avattr_type_string,
			avattr_type_int,
			avattr_type_double>
	avattr_types;
	/*
	 	 insert map
	 */
	void set(avattr_type_string &&key,
			avattr_types  &&value)
	{
		_map.insert(
				std::make_pair(key,
						value)
		);
	}
	void set(char const *key, avattr_types  &&value)
	{
		_map.insert(
				std::make_pair(avattr_type_string(key),
				value));
	}

	void set(avattr_type_string &key,
			avattr_type_string &first,
			const avattr_type_int second,
			const avattr_type_double third)
	{
		set (key.c_str(), first.c_str(), second, third);
	}
	void set(char const *key, char const *str, int n, double d)
	{
		_map.insert(
				std::make_pair(avattr_type_string(key),
				avattr_types(avattr_type_string(str), n, d)));
	}
	void set(avattr_type_string &&key,
			avattr_type_string &&first,
			const avattr_type_int second,
			const avattr_type_double third)
	{
		set(static_cast<avattr_type_string &&>(key),
				std::make_tuple(first,
						second,
						third));
	}
	void reset(char const *key, char const *str, int n, double d)
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
			it.second = avattr_types(avattr_type_string(str), n, d);


				break;
			}
		}
			
	}

	/*
	 	 get map
	 */

	avattr_type_string get_str(char const *key,
			const avattr_type_string &def = avattr_type_string("")) const
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return std::get<0>(it.second);
			}
		}
		return def;
	}
	char const *get_char(char const *key,
			char const *def = nullptr)  const
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return std::get<0>(it.second).c_str();
			}
		}
		return def;
	}
	int get_int(char const *key,
			int def = 0)  const
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return std::get<1>(it.second);
			}
		}
		return def;
	}
	double get_double(char const *key,
			double def = 0.0)  const
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return std::get<2>(it.second);
			}
		}
		return def;
	}
	avattr_types  get(char const *key)
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return (it.second);
			}
		}
		return avattr_types();		
	}


	bool notfound(char const *key)  const
	{
		bool has = false;
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				has = true;
				break;
			}
		}

		return !has;
	}

	bool has_frame_video() const
	{
		return !notfound(avattr_key::frame_video);
	}
	bool has_frame_audio()  const
	{
		return !notfound(avattr_key::frame_audio);
	}
	bool has_frame_any() const
	{
		return has_frame_video() ||
				has_frame_audio();
	}
	avattr()
	{
		_map.clear();
	}
	virtual ~avattr(){}
private:
	std::map<
	std::string,
	avattr_types
	> _map;
};


