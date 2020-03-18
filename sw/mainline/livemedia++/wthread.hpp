#pragma once



/*
 	 	 workqueue thread interface class
 */
typedef struct WQ workqueue;
typedef std::function<workqueue *(_dword, _dword)> function_workqueue_open;
typedef std::function<void (workqueue ** )> function_workqueue_close;
typedef std::function<_dword (workqueue *, void **, _dword *, _dword )> function_workqueue_recv;
typedef std::function<_dword (workqueue *, void *, _dword , _dword, int  )>  function_workqueue_send;

template <typename _Class>
struct _wthread_functor
/*
 	 	 workqueue thread 'main  functor'
 */
{

	template <typename ...Args>
	void operator () (
			char const *name,
			workqueue *_wq,
			function_workqueue_recv recv,
			_dword time,
			Args...args)
	{
		DECLARE_LIVEMEDIA_NAMEDTHREAD(name);

		/*
		 	 	 we create object to stack memory
				default constructor has no '()'
		 */
		if (sizeof... (args) <= 0)
		{
			_Class t;
			s(_wq, recv, time, t);
			return;
		}

		_Class t(args...);

		s(_wq, recv, time, t);
	}
private:
	void s(workqueue *_wq,
			function_workqueue_recv &recv,
			_dword time,
			 _Class &o)
	{
		_dword r = WQ_FAIL;
		void * par = NULL;
		_dword size= 0;
		_dword qtime = time;
		do
		{
			/*
			 	 	 receving event from queue
			 */
			r = recv(_wq, &par, &size, qtime);
			if(r == WQ_FAIL)
			{
				DECLARE_THROW(true, "workqueue has return fail");
			}

			/*
			 	 	 notify to object
			 */
		}while(o(r, par, size, &qtime));
	}
};

template <typename _Class>
class wthread
{
private:
	std::thread *_th;
	workqueue *_wq;
	char const *_wt_name;

	/*
	 	 	 get workqueue operation functions
	 */
	const function_workqueue_open open;
	const function_workqueue_close close;
	const function_workqueue_recv recv;
	const function_workqueue_send send;


public:
	wthread( wthread &&rhs):
		_th(nullptr),
		_wq(NULL),
		_wt_name(nullptr),
		open(WQ_open),
		close(WQ_close),
		recv(WQ_recv),
		send(WQ_send)
	{
		operator = (static_cast<wthread &&>(rhs));
	}
	wthread(_dword len, _dword size, char const *wt_name) :
		_th(nullptr),
		_wq(NULL),
		_wt_name(wt_name),
		open(WQ_open),
		close(WQ_close),
		recv(WQ_recv),
		send(WQ_send)
	{

		_wq = open(size , len );
		DECLARE_THROW(!_wq, "can't create workqueue");

	}
	virtual ~wthread()
	{
		end();
	}
	template <typename ... Args>
	void start(_dword timeout, Args ...args)
	/*
	 	 	 start thread
	 */
	{
		_th = new std::thread(_wthread_functor<_Class>(),
				_wt_name,
				_wq,
				recv,
				timeout,
				args...);
	}
	_dword  sendto(void *p,
			_dword n,
			_dword timeout = INFINITE)
	/*
	 	 	 sending event 'nonblocking' mode
	 */
	{
		send(_wq, p, n, timeout, 0);
		return WQOK;
	}
	_dword sendto_wait( void *p,
			_dword n,
			_dword timeout = INFINITE)
	/*
	 	 	 sending event 'blocking' mode
	 	 	 return when 'WQ_recv' return the this event
	 */
	{
		send(_wq, p, n, timeout, WQ_SEND_FLAGS_BLOCK);
		return WQOK;
	}
	wthread & operator = ( wthread &&rhs)
	{
		this->_th = rhs._th;
		this->_wq = rhs._wq;
		this->_wt_name = rhs._wt_name;
		
		rhs._th = nullptr;
		rhs._wq = nullptr;
		rhs._wt_name = nullptr;
		return *this;

	}
	void end()
	{
		if(_th)
		{
			_th->join();
			delete _th;
			_th = nullptr;
		}
		if(_wq)
		{
			close(&_wq);
			_wq = nullptr;
		}
	}


	bool exec()
	{
		return _th || _wq ? true : false;
	}
};


