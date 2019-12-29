#pragma once


struct playback_inst
{
	avattr _attr;
	playback_inst(const avattr &attr) : _attr(attr){}
	virtual ~playback_inst (){};
	virtual void connectionwait(){};
	virtual void pause()  = 0;
	virtual void resume()  = 0;
	virtual void seek(double incr)  = 0;
	virtual bool has(avattr::avattr_type_string &&key)  = 0;
	virtual void play()   = 0;
	virtual int take(const std::string &title, pixel &output) = 0;
	virtual int take(const std::string &title, pcm_require &output) = 0;
	static playback_inst * create_new(const avattr &attr,
			char const *name,
			unsigned time = 0,
			char const *id = nullptr,
			char const *pwd = nullptr);
};


class playback
{
private:
	playback_inst *_inst;
public:

	/*
	 	 	 constructor local
	 */
	playback(const avattr &attr, char const *name) :
		_inst(playback_inst::create_new(attr, name))
	{ throw_if()(!_inst, "can't create playback inst");}

	/*
	 	 	 constructor rtsp
	 */
	playback(const avattr &attr,
						char const *url,
						unsigned connectiontime,
						char const *auth_id = nullptr,
						char const *auth_pwd = nullptr) :
		_inst(playback_inst::create_new(attr, url, connectiontime, auth_id, auth_pwd))
	{ throw_if()(!_inst, "can't create playback inst");}

	virtual ~playback()
	{ if(_inst)delete _inst;_inst = nullptr;}
	void pause()
	{_inst->pause();}
	void resume()
	{_inst->resume();}
	void seek(double incr)
	{_inst->seek(incr);}
	bool has(avattr::avattr_type_string &&key)
	{return _inst->has(static_cast<avattr::avattr_type_string &&>(key));}
	void play(){_inst->play();}

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




