#pragma once
struct playback_inst
{
	avattr _attr;
	playback_inst(const avattr &attr) : _attr(attr){}
        virtual ~playback_inst (){}
        virtual void connectionwait(){}
	virtual void pause()  = 0;
	virtual void resume()  = 0;
	virtual void seek(double incr)  = 0;
	virtual bool has(avattr::avattr_type_string &&key)  = 0;
	virtual void play()   = 0;
	virtual int take(const std::string &title, pixel &output) = 0;
	virtual int take(const std::string &title, pcm_require &output) = 0;
        virtual duration_div duration() = 0;
};
