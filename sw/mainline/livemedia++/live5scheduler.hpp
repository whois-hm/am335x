#pragma once

#define LIVE5_BUFFER_SIZE	1 * 1024 * 1024

template
<typename Callable>
class live5scheduler
{
public:
	typedef std::function<void (live5scheduler<Callable> *, void *)> usreventfunction;
	typedef std::tuple< usreventfunction, 	/*for use user callback*/
			live5scheduler<Callable> *, 		/*for use this class pointer*/
			void *		/*for use registed user pointer*/
			> live5event;
	typedef std::tuple<unsigned,	/*use user trigger id*/
			EventTriggerId, 		/*use for live555 trigger id*/
			live5event			/*use for trigger param*/
			>live5event_pair;
	static const int live5scheduler_trigger_id_close = 0;
	static const int live5scheduler_trigger_id_user_start = 1;
private:
	/*
	 	 if you can want loop at other thread
	 */
	std::thread *_th;
	char _loop;
	TaskScheduler *_scheduler;
	UsageEnvironment *_env;
	Callable *_cal;
	semaphore _wait_sema;
	std::vector<live5event_pair> _evts;

	static void live5event_function(void *ptr)
	{
		live5event val = *(live5event *)ptr;
		std::get<0>(val)(std::get<1>(val),
				std::get<2>(val));
	}
	bool runable()
	{
		return _scheduler != nullptr &&
				_env != nullptr &&
				_cal!= nullptr;
	}
	template <typename ...Args >
	void loop(Args ...args)
	{
		_scheduler = BasicTaskScheduler::createNew();
		DECLARE_THROW(!_scheduler, "can't create live5 scheduler");

		_env = BasicUsageEnvironment::createNew(*_scheduler);
		DECLARE_THROW(!_env, "can't create live5 environment");
		for(auto &it : _evts)
		{
			EventTriggerId &id = std::get<1>(it);
			id = _scheduler->createEventTrigger(live5scheduler::live5event_function);
			DECLARE_THROW(id == 0, "can't create live5 eventtrigger");
		}
		_cal = new Callable(*_env, args...);
		DECLARE_THROW(!_cal, "can't create live5 callable");

		SEMA_unlock(&_wait_sema);
		_scheduler->doEventLoop(&_loop);

		if(_cal) delete _cal;
		if(_env)_env->reclaim();
		if(_scheduler)delete _scheduler;
		_env = nullptr;
		_scheduler = nullptr;
		_cal = nullptr;
	}
public:
	Callable *refcallable()
	{
		/*
		 	 warnning !
		 	 this value create 'other thread' or this loop.
		 	 judgement is your

		 	 our make this reference fuction purpose that
		 	 'trigger' event and use live5scheduler *.
		 	 this case is always valid.
		 */
		return _cal;
	}
	template <typename _functor>
	bool register_trigger(unsigned id, _functor &&_f, void *ptr)
	{
		if(runable())
		{
			return false;
		}
		_evts.push_back(std::make_tuple(id,
				0,/*temp value*/
				std::make_tuple(_f,
						this,
						ptr)));
		return true;
	}
	bool trigger(unsigned id)
	{
		if( _scheduler == nullptr ||
				_env == nullptr ||
				_cal == nullptr)
		{
			return false;
		}
		for(auto &it : _evts)
		{
			if(std::get<0>(it) == id)
			{
				_scheduler->triggerEvent(std::get<1>(it), &std::get<2>(it));
				return true;
			}
		}
		return false;
	}
	live5scheduler() :
		_th(nullptr),
		_loop(0),
		_scheduler(nullptr),
		_env(nullptr),
		_cal(nullptr)
	{
		register_trigger(live5scheduler_trigger_id_close,
				[&](live5scheduler<Callable> *, void *)->void{_loop = 1;},
				nullptr );
	}
	~live5scheduler()
	{
		/*
		 	 using sync '_wait_sema'
		 */

		trigger(live5scheduler_trigger_id_close);

		if(_th)
		{
			_th->join();
			delete _th;
			_th = nullptr;
			SEMA_close(&_wait_sema);
			return;
		}

		if(_cal) delete _cal;
		if(_env)_env->reclaim();
		if(_scheduler)delete _scheduler;

		_env = nullptr;
		_scheduler = nullptr;
		_cal = nullptr;
	}

	template <typename ...Args >
	void start(bool at_another_thread,
			Args ...args)
	{
		if(at_another_thread)
		{

			DECLARE_THROW(WQOK != SEMA_open(&_wait_sema, 0, 1), "can 't create semaphore");

			_th = new std::thread(
					[&]()->void{ 
					DECLARE_LIVEMEDIA_NAMEDTHREAD("live5 scheduler");
					loop(args...
				);
			});
			SEMA_lock(&_wait_sema, INFINITE);
			return;
		}
		loop(args...);
	}
};

