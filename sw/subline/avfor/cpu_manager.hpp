#pragma once
/*--------------------------------------
	running for check cpu usage
--------------------------------------*/
struct cpu_manager_par
{
	cpu_manager_par(double cpu, 
		double sys,
		double usr,
		double idle) : 
		_cpu(cpu),
			_sys(sys),
			_usr(usr),
			_idle(idle){}

	double _cpu;
	double _sys	;
	double _usr	;
	double _idle	;
};

class cpu_manager;
class cpu_usage
{
	friend class cpu_manager;
	enum command
	{
		start = 0,
		ing,
		stop,
		close,
		get
	};
	struct par
	{
		enum command cmd;
		unsigned interval;
		bool *get;
	};
	enum cpu_jiffytype
	{
		cpu_jiffytype_cpu,
		cpu_jiffytype_sys,
		cpu_jiffytype_usr,
		cpu_jiffytype_max
	};
	
	avfor_context *_ctx;
	ui * _ui;
	unsigned _interval;
	enum command _state;


	clock_t cpu_jiffy[2][cpu_jiffytype_max];
		void put_main(double cpu, 
		double sys, 
		double usr,
		double idle)
	{
		
		struct pe_user u;
		u._code = custom_code_cpu_usage_notify;
		u._ptr = (void *)new cpu_manager_par(cpu, sys, usr, idle);
		_ui->write_user(u);
		
	}
	void update_usage()
	{
		struct tms timesample;
		cpu_jiffy[1][cpu_jiffytype_cpu] = times(&timesample);
		cpu_jiffy[1][cpu_jiffytype_sys] = timesample.tms_stime;
		cpu_jiffy[1][cpu_jiffytype_usr] = timesample.tms_utime;

		clock_t sys =  cpu_jiffy[1][cpu_jiffytype_sys] - cpu_jiffy[0][cpu_jiffytype_sys];
		clock_t usr =  cpu_jiffy[1][cpu_jiffytype_usr] - cpu_jiffy[0][cpu_jiffytype_usr];
		clock_t cpu =  cpu_jiffy[1][cpu_jiffytype_cpu] - cpu_jiffy[0][cpu_jiffytype_cpu];

		cpu_jiffy[0][cpu_jiffytype_cpu] = cpu_jiffy[1][cpu_jiffytype_cpu];
		cpu_jiffy[0][cpu_jiffytype_sys] = cpu_jiffy[1][cpu_jiffytype_sys];
		cpu_jiffy[0][cpu_jiffytype_usr] = cpu_jiffy[1][cpu_jiffytype_usr];	

		double cpu_per = (double)((double)(sys + usr) / (double)cpu) * (double)100.0;
		if(cpu_per > 100.0)cpu_per = 100.0;
		if(cpu_per < 0)cpu_per = 0.0;
		double sys_per = (double)((double)(sys) / ((double)sys + (double)usr)) * (double)100.0;
		if(sys_per > 100.0)sys_per = 100.0;
		if(sys_per < 0)sys_per = 0.0;
		double usr_per = (double)((double)(usr) / ((double)sys + (double)usr)) * (double)100.0;
		if(usr_per > 100.0)usr_per = 100.0;
		if(usr_per < 0)usr_per = 0.0;
		double idle_per = 100.0 - cpu_per;
		if(idle_per > 100.0)idle_per = 100.0;
		if(idle_per < 0)idle_per = 0.0;
		printf("%d %d %d %f %f %f %f\n", sys , usr , cpu , cpu_per, sys_per, usr_per, idle_per);
		put_main(cpu_per, sys_per, usr_per, idle_per);
		
	}
	void start_usage(unsigned interval)
	{
		if(_state == stop )
		{
			_interval = interval;
			update_usage();
		}
		_state = ing;
	}
	void stop_usage()
	{
		_state = stop;
	}
	void ing_usage()
	{
		_state = ing;
		update_usage();
	}
	
public:
	cpu_usage() {}
	cpu_usage(avfor_context *ctx,
	ui * ui) : 
	_ctx(ctx), 
	_ui(ui), 
	_interval(0),
	_state(stop)
	{
		/*at first */
		struct tms timesample;
		cpu_jiffy[0][cpu_jiffytype_cpu] = times(&timesample);
		cpu_jiffy[0][cpu_jiffytype_sys] = timesample.tms_stime;
		cpu_jiffy[0][cpu_jiffytype_usr] = timesample.tms_utime;


		cpu_jiffy[1][cpu_jiffytype_cpu] = cpu_jiffy[0][cpu_jiffytype_cpu];
		cpu_jiffy[1][cpu_jiffytype_sys] = cpu_jiffy[0][cpu_jiffytype_sys];
		cpu_jiffy[1][cpu_jiffytype_usr] = cpu_jiffy[0][cpu_jiffytype_usr];		
	}
	~cpu_usage(){}

	bool operator ()
		(_dword r, void *par, _dword size, _dword *qtime)
	{
		if(r == WQOK)
		{
			struct par *_param = (struct par *)par;
			if(_param->cmd == start)
			{
				start_usage(_param->interval);
				*qtime = _param->interval;
			}
			else if(_param->cmd == stop)
			{
				stop_usage();
				*qtime = INFINITE;
			}
			else if(_param->cmd == get)
			{
				*_param->get = _state == ing ? true : false;
			}
			else if(_param->cmd == close)
			{
				return false;	
			}
			return true;
		}
		if(r == WQ_TIMED_OUT)
		{
			ing_usage();
			return true;
		}
		return false;
	}		
};
class cpu_manager : 
	public wthread<cpu_usage>
{

public:

	cpu_manager(avfor_context *ctx,
	ui * ui_int) : 
	wthread(10, sizeof(cpu_usage:: par))
	{
		start(INFINITE, ctx, ui_int);
	}
	~cpu_manager()
	{
		cpu_usage:: par p;
		p.cmd =  cpu_usage::close;
		
		 sendto(&p,
			sizeof(cpu_usage:: par));
	}
	void start_cpu_usage(unsigned interval)
	{
		cpu_usage:: par p;
		p.cmd =  cpu_usage::start;
		p.interval = interval;
		
		 sendto(&p,
			sizeof(cpu_usage:: par));
	}
	void stop_cpu_usage()
	{
		cpu_usage:: par p;
		p.cmd =  cpu_usage::stop;
		
		 sendto_wait(&p,
			sizeof(cpu_usage:: par));
	}
	bool state()
	{
		bool s = false;
		cpu_usage::par p;
		p.cmd =  cpu_usage::get;
		p.get = &s;
		
		 sendto_wait(&p,
			sizeof(cpu_usage:: par));
		 return s;
	}
	
};

