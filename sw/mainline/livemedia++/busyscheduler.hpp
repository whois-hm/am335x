#pragma once

/*
 	 	 useful class for safety loop

 	 	 can  yield cpu  to other thread use operator ()
 */
class busyscheduler
{
public:
	busyscheduler() :
		_lastcalltime(std::chrono::system_clock::now()) { }
	virtual ~busyscheduler() { }
	void operator() (std::chrono::milliseconds  &&calltime,	 /*calling interval*/
			std::chrono::milliseconds &&pendingtime/*wait time*/
			)
	{
		std::chrono::system_clock::time_point curcalltime =  std::chrono::system_clock::now();
		std::chrono::milliseconds elapsetime = std::chrono::duration_cast<std::chrono::milliseconds>(curcalltime - _lastcalltime);

		if(elapsetime.count() >= calltime.count() )
		{
			std::this_thread::sleep_for(pendingtime);
		}
		_lastcalltime = curcalltime;

	}

private:
	std::chrono::system_clock::time_point _lastcalltime;
};

