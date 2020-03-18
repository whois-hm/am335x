#pragma once

class gpio_manager;
class am335x_gpio_control
{
friend class gpio_manager;
private : 
	enum command
	{
		cpu_usage,
		cpu_usage_stop,
		close
	};
	struct par
	{
		enum command cmd;
		double percent;
	};
	/*
		mapping code am335x_gpio.h
	*/
	enum bank
	{
		rgb_bank1_red = 0,
		rgb_bank1_green = 1,
		rgb_bank1_blue = 2,

		rgb_bank2_red = 3,
		rgb_bank2_green = 4,
		rgb_bank2_blue = 5,		
	};
	enum direction
	{
		high = 0,
		low = 1
	};
	struct gpio_request
	{
		enum bank t;
		enum direction d;
	};
	void led_off(int bank)
	{
		if(bank == 1)
		{
			led_off(rgb_bank1_red);
			led_off(rgb_bank1_green);
			led_off(rgb_bank1_blue);
		}
		if(bank == 2)
		{
			led_off(rgb_bank2_red);
			led_off(rgb_bank2_green);
			led_off(rgb_bank2_blue);
		}
	}
	void led_off(enum bank t)
	{
		struct gpio_request req;
		req.t = t;
		req.d = high;
		write(_fd, &req, 1);	
	}
	void led_on(int bank, bool r, bool g, bool b)
	{
		if(bank == 1)
		{
			r ? led_on(rgb_bank1_red) : led_off(rgb_bank1_red);
			g ? led_on(rgb_bank1_green) : led_off(rgb_bank1_green);
			b ? led_on(rgb_bank1_blue) : led_off(rgb_bank1_blue);
		}		
		if(bank == 2)
		{
			r ? led_on(rgb_bank2_red) : led_off(rgb_bank2_red);
			g ? led_on(rgb_bank2_green) : led_off(rgb_bank2_green);
			b ? led_on(rgb_bank2_blue) : led_off(rgb_bank2_blue);
		}	
	}
	void led_on(enum bank t)
	{
		struct gpio_request req;
		req.t = t;
		req.d = low;
		write(_fd, &req, 1);	
	}
	bool get_binary_rand()
	{
		return (rand() % 2 == 0) ? false : true;
	}
	int _fd;
public:
	am335x_gpio_control() : _fd(-1){}
	am335x_gpio_control(int fd) : 
		_fd(fd)
	{
		srand(time(nullptr));
		led_off(1);
		led_off(2);
	}
	~am335x_gpio_control()
	{
		led_off(rgb_bank1_red);
		led_off(rgb_bank1_green);
		led_off(rgb_bank1_blue);
		led_off(rgb_bank2_red);
		led_off(rgb_bank2_green);
		led_off(rgb_bank2_blue);
	}

	bool operator ()
		(_dword r, void *par, _dword size, _dword *qtime)
	{
		if(r == WQOK)
		{
			am335x_gpio_control::par *param = (am335x_gpio_control::par *)par;
			if(param->cmd == cpu_usage)
			{
				if(param->percent >= 0.0 && param->percent < 30.0)
				{
					led_on(2, false, true, false);
				}
				else if(param->percent >= 30.0 && param->percent < 50.0)
				{
					led_on(2, false, false, true);
				}
				else
				{
					led_on(2, true, false, false);
				}
				return true;
			}
			else if(cpu_usage_stop == param->cmd)
			{
				led_off(2);
				return true;
			}
			else
			{
				return false;
			}
		}
		if(r == WQ_TIMED_OUT)
		{
			bool r = get_binary_rand();
			bool g = get_binary_rand();
			bool b = get_binary_rand();

			led_on(1, r, 
				g, 
				b);
			Time_sleep(1);
			led_off(1);
			return true;
		}
		return false;
	}		
};
class gpio_manager : 
	public wthread<am335x_gpio_control>,
	public manager
{
private:
		int _fd;
public:
	gpio_manager(avfor_context  *avc,
			ui *interface) :
	wthread(10, sizeof(am335x_gpio_control:: par), "avfor gpio manager"),
	manager(avc, interface),
	_fd(-1)
	{		
		_fd = ::open("/dev/am335x_gpio", O_RDWR);
	}
	~gpio_manager()
	{
			am335x_gpio_control:: par p;
		p.cmd =  am335x_gpio_control::close;
		
		 sendto(&p,
			sizeof(am335x_gpio_control:: par));
		 
		if(_fd >= 0)
		{
			::close(_fd);
		}
		_fd = -1;
	}

	virtual bool ready()
	{
		/*
			always true, for code interface 
		*/
		if(_fd >= 0)
		{
				start(1000, _fd);
		}
		return true;
	}
	virtual void operator()(ui_event &e)
	{
		if(_fd >=0 && 
			e.what() == platform_event_user)
		{
			if(e.user()->_code == custom_code_util_cpu_usage_notify)
			{
				struct cpu_manager_par *par = (struct cpu_manager_par *)e.user()->_ptr;
				am335x_gpio_control::par p;
				p.cmd =  am335x_gpio_control::cpu_usage;
				p.percent = par->_cpu;		
				sendto(&p,sizeof(am335x_gpio_control::par));
			}
			if(e.user()->_code == custom_code_util_cpu_usage_stop_notify)
			{
				am335x_gpio_control::par p;
				p.cmd =  am335x_gpio_control::cpu_usage_stop;	
				sendto(&p,sizeof(am335x_gpio_control::par));				
			}


			
				
		}
	}

};

