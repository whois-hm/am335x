#pragma once
#include "tslib.h"




class touch
{
private:
	struct ts_sampling
	{
		struct ts_sample _samp;
		std::chrono::system_clock::time_point _clock;
		ts_sampling(struct ts_sample &samp) : 
			_samp(samp),
			_clock(std::chrono::system_clock::now())

	 	{}
	 	ts_sampling(triple_int &samp)
	 	{
	 		_samp.x = std::get<0>(samp);
			_samp.y = std::get<0>(samp);
			_samp.pressure = std::get<0>(samp);
	 	}
		~ts_sampling(){}
		bool range_match(struct ts_sample &samp, int errrange)
		{
			int dif_x = std::abs(_samp.x - samp.x);
			int dif_y = std::abs(_samp.y - samp.y);
			
			return  dif_x < std::abs(errrange) && 
				dif_y < std::abs(errrange);
		}
		bool clock_match( ts_sampling & s, std::chrono::microseconds c)
		{	

			auto dif = std::chrono::duration_cast<std::chrono::microseconds>(s._clock - _clock);
			if(dif > c)
			{
				return true;
			}
			return false;
		}
		
	};

private:
	struct tsdev *_ts;
	bool _nonblock;
	unsigned _last_samp_has_up;
	std::chrono::microseconds _debouce_time_clock_max;
	unsigned _debouce_sample_max;
	int _debounce_range;
	std::list<ts_sampling> _debounce_sampling;	
	triple_int _last_samp;
	
	bool enabled_debounce_mode()
	{
		return _debouce_sample_max > 0;
	}
	bool chk_debounce_error_range(struct ts_sample &samp)
	{
		auto &pick = _debounce_sampling.back();
		return pick.range_match(samp,_debounce_range);			
	}
	int chk_samp_direction(struct ts_sample &samp)
	{
		if(samp.pressure > 0 ) return 1;/*down*/
		return 0;/*up*/
	}
	bool chk_samp_clock(struct ts_sample &samp)
	{
		ts_sampling s(samp);
		auto &pick = _debounce_sampling.front();
		return pick.clock_match(s, _debouce_time_clock_max);		
	}
	triple_int calc_samp_avg()
	{

		int avg_x = 0;
		int avg_y = 0;
		int avg_p = 0;
		int size = _debounce_sampling.size();
		for(auto &it : _debounce_sampling)
		{
			avg_x += it._samp.x;
			avg_y += it._samp.y;
			avg_p += it._samp.pressure;
		}
		return std::make_tuple(int(avg_x / size), (int)(avg_y / size), (int)(avg_p / size));
	}
	int debounce(struct ts_sample &samp, triple_int & out)
	{
		if(!enabled_debounce_mode())
		{
			out = std::make_tuple(samp.x, samp.y, samp.pressure);
			return 0;
		}
		/*debounce check*/
		if(!chk_samp_direction(samp))
		{
			if(_last_samp_has_up)
			{
				/*don't check range.  
			    		because occur up event position where 
			    		no sampling down press event position
				*/
				//ts_sampling s1(_last_samp);
				//s1.range_match(samp, _debounce_range);
				out = std::make_tuple(std::get<0>(_last_samp),
				std::get<1>(_last_samp),
				0);
				_last_samp_has_up = 0;
				return 0;
			}
			return 1;/*dump return : throwing*/
		}

		if(_debounce_sampling.empty())
		{
			/*new point found from list*/
			_debounce_sampling.push_back(ts_sampling(samp));
			return _debounce_sampling.size();
		}

		if(!chk_debounce_error_range(samp))
		{
			/*new point found from range*/
			_debounce_sampling.clear();
			_debounce_sampling.push_back(ts_sampling(samp));
			return _debounce_sampling.size();
		}

		if(chk_samp_clock(samp))
		{
			/*new point found from clock*/
			_debounce_sampling.clear();
			_debounce_sampling.push_back(ts_sampling(samp));
			return _debounce_sampling.size();
		}
		
		/*sampling*/
		_debounce_sampling.push_back(ts_sampling(samp));
		if(_debounce_sampling.size() < _debouce_sample_max)
		{

			return _debounce_sampling.size();
		}
		/*now have event*/
		_last_samp_has_up = 1;		
		_last_samp = out = calc_samp_avg();
		_debounce_sampling.clear();
		return 0;
		
	}
public:
	touch(bool nonblock = false) :
		_ts(nullptr),
		_nonblock(nonblock),
		_last_samp_has_up(0),
		_debouce_time_clock_max(std::chrono::microseconds(0)),
		_debouce_sample_max(0),
		_debounce_range(0)
	{
		_ts = ts_setup(nullptr, nonblock ? 1 : 0);
		DECLARE_THROW(!_ts, "can't setup tslib");
	}	
	virtual ~touch()
	{
		if(_ts)
		{
			ts_close(_ts);
		}

		_ts = nullptr;
	}

	void enable_debounce_mode(unsigned max_micro = 1000, 
		unsigned max_sampling = 1,
		int max_range = 0)
	{
		if(max_sampling <= 0)
		{
			return;
		}
		_debouce_sample_max = max_sampling;
		_debounce_range = max_range;
		_debouce_time_clock_max = std::chrono::microseconds(max_micro);
	}


	int fd()
	{
		return ts_fd(_ts);
	}
	bool is_nonblock()
	{
		return _nonblock;	
	}

	int read_samp(triple_int & sampout)
	{
		/*
		 - value has error occur
		 0 value has read ok, read parameter samp
		 + value debounce sampling 
		*/
		struct ts_sample samp;
		int res = -1;
		res = ts_read(_ts, &samp, 1);
		if(res < 0  || 
			res != 1)
		{
			return -1;
		}
		return debounce(samp, sampout);
	}

	
};
