#pragma once
class playback
{
protected:
	playback_inst *_inst;
public:
	static playback_inst * create_new(const avattr &attr,
			char const *name,
			unsigned time = 0,
			char const *id = nullptr,
			char const *pwd = nullptr)
	{
		playback_inst *inst = nullptr;
		bool res = true;
		if(!attr.has_frame_any())
		{
			return nullptr;
		}
		if(!inst)
		{
#if defined have_uvc
			if(strlen (name) > 4 &&
					name[0] == '/' &&
					name[1] == 'd' &&
					name[2] == 'e' &&
					name[3] == 'v')
			{
				inst = new uvc_playback(attr, name);
			}
#endif
		}
		if(!inst)				
		{
			if(!access(name, 0))
			{
				inst = new local_playback(attr, name);
			}
				
		}



		if(!inst)
		{
			inst = new rtsp_playback(attr, name, time, id, pwd);
		}

		if(inst)
		{
			res = inst->connectionwait();
		}
		if(!res)
		{
			if(inst)
			{
				delete inst;
				inst = nullptr;
			}
		}
		return inst;
	}
	/*
	 	 	 constructor local
	 */
	playback(const avattr &attr, char const *name) :
                _inst(create_new(attr, name))
	{ DECLARE_THROW(!_inst, "can't create playback inst");}

	/*
	 	 	 constructor rtsp
	 */
	playback(const avattr &attr,
						char const *url,
						unsigned connectiontime,
						char const *auth_id = nullptr,
						char const *auth_pwd = nullptr) :
                _inst(create_new(attr, url, connectiontime, auth_id, auth_pwd))
	{ DECLARE_THROW(!_inst, "can't create playback inst");}

	virtual ~playback()
	{ if(_inst)delete _inst;_inst = nullptr;}
	void record(char const *file = nullptr)
	{ _inst->record(file); }
	void pause()
	{_inst->pause();}
	void resume(bool closing = false)
	{_inst->resume(closing);}
	void seek(double incr)
	{_inst->seek(incr);}
	bool has(avattr::avattr_type_string &&key)
	{return _inst->has(static_cast<avattr::avattr_type_string &&>(key));}
	void play(){_inst->play();}
	enum AVMediaType get_master_clock(){return _inst->get_master_clock();}
    duration_div duration(){return _inst->duration();}

	/*
	 	 	 if 0 > 1 take frame
	 	 	 or 0 no ready frame
	 	 	 or < 0 err
	 */
	template <typename _T>
	int  take(_T &d,
			const std::string &title = ""/*for rtsp stream*/
			)
	{
		return _inst->take(title, d);
	}
};




