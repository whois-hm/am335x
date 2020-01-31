#pragma once
#include "tslib.h"
class touch
{
private:
	struct tsdev *_ts;
	bool _nonblock;
	struct ts_sample _samp;
public:
	touch(bool nonblock = false) :
		_ts(nullptr),
		_nonblock(nonblock)
	{
		memset(&_samp, 0, sizeof(_samp));
		_ts = ts_setup(nullptr, nonblock ? 1 : 0);
		throw_if t;
		t(!_ts, "can't setup tslib");
	}	
	virtual ~touch()
	{
		if(_ts)
		{
			ts_close(_ts);
		}

		_ts = nullptr;
	}
	int fd()
	{
		return ts_fd(_ts);
	}
	bool is_nonblock()
	{
		return _nonblock;	
	}
	bool wait_samp()
	{
		int res = -1;
		res = ts_read(_ts, &_samp, 1);
		if(res < 0  || 
			res != 1)
		{
			return false;
		}
		return true;
	}
	triple_int get_samp()
	{
		return triple_int(_samp.x, 
			_samp.y,
			_samp.pressure);
	}
	
};
