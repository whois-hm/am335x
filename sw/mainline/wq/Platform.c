#include "_WQ.h"
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
 static void timeconv_abs(_dword ms, struct timespec *time)
 {
		struct timespec cur_clock;
		_dword total_clock = 0;
		int carry = 0;

		if(!time)
		{
			return;
		}
		clock_gettime(CLOCK_REALTIME, &cur_clock);

		total_clock = ms + (cur_clock.tv_nsec / 1000000);

		carry = total_clock / 1000;

		time->tv_sec = cur_clock.tv_sec += carry;
		time->tv_nsec = cur_clock.tv_nsec  = ((total_clock % 1000) * 1000000) + cur_clock.tv_nsec % 1000000;
 }

static void *__start_routine (void * p)
{
	struct BackGround *b = (struct BackGround *)p;
	b->func(b->p);
	return NULL;
}

#else

#endif


int Platform_thread_open(struct BackGround *b)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return WQ_INVALID;
#elif defined (_platform_linux)

	if(!b)
	{
		return WQEINVAL;
	}
	if(pthread_create(&b->t_id, NULL, __start_routine, b) != 0)
	{
		return WQENOMEM;
	}
	return WQOK;

#else
	return WQ_INVALID;
#endif
}
void Platform_thread_close(struct BackGround *b)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return ;
#elif defined (_platform_linux)

	if(!b)
	{
		return ;
	}
	pthread_join(b->t_id, NULL);
#else
	return ;
#endif
}
void  Platform_sleep(_dword time)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return ;
#elif defined (_platform_linux)

		usleep(time * 1000);
#else
	return ;
#endif
}
_dword Platform_tick_count()
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return 0;
#elif defined (_platform_linux)
	struct timeval v;
	gettimeofday(&v, NULL);

	return (v.tv_sec * 1000) + (v.tv_usec / 1000);

#else
	return 0;
#endif
}
void  Platform_thread_id(threadid *id)
{
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
	if(id) *id = pthread_self();
#else

#endif
}
int Platform_semephore_open(semaphore *sema, _dword init, _dword max)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return -1;
#elif defined (_platform_linux)
	return sema ? sem_init(sema,  0 , init) : -1;
#else
	return -1;
#endif
}
void  Platform_semaphore_close(semaphore *sema)
{
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
	if(sema) sem_destroy(sema);
#else
#endif
}

_dword Platform_semaphore_lock(semaphore *sema, _dword time)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return OBJECT_INVALID;
#elif defined (_platform_linux)
	int i = -1;
	if(!sema)
	{
		return WQ_INVALID;
	}
	if(time == 0)
	{
		if(sem_trywait(sema) >= 0)
		{
			return WQ_SIGNALED;
		}
		else
		{
			i = errno;
		}
	}
	else if(time == INFINITE)
	{
		if(sem_wait(sema) >= 0)
		{
			return WQ_SIGNALED;
		}
		else
		{
			i = errno;
		}
	}
	else
	{
		struct timespec timespec;
		timeconv_abs(time, &timespec);
		if(sem_timedwait(sema, &timespec) < 0)
		{
			i = errno;
			//printf("errno : %d", errno);
		}
		else
		{
			return  WQ_SIGNALED;
		}
	}

	if(i == EAGAIN ||
			i == ETIMEDOUT)
	{
		return WQ_TIMED_OUT;
	}

	return WQ_FAIL;
#else
#endif
}
void Platform_semaphore_unlock(semaphore *sema)
{
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
	if(sema) sem_post(sema);
#else
#endif
}

int Platform_criticalsection_open(criticalsection *cs)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return GEINVAL;
#elif defined (_platform_linux)
	return cs ? pthread_mutex_init(cs, NULL) : WQEINVAL;
#else
	return GEINVAL;
#endif
}
void  Platform_criticalsection_close(criticalsection *cs)
{
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
	if(cs) pthread_mutex_destroy(cs);
#else
#endif
}


_dword Platform_criticalsection_lock(criticalsection *cs, _dword time)
{
#if defined  (_platform_mingw) || defined(_platform_win)
	return WQ_INVALID;
#elif defined (_platform_linux)
		int i = -1;
		if(!cs)
		{
			return WQ_INVALID;
		}
		if(time == 0)
			i = pthread_mutex_trylock(cs);
		else if(time == INFINITE)
			i = pthread_mutex_lock(cs);
		else
		{
			struct timespec timeabs;
			timeconv_abs(time, &timeabs);
			i = pthread_mutex_timedlock(cs, &timeabs);
		}
		if(i != 0)
		{
			if(i == EAGAIN || i == ETIMEDOUT) i = WQ_TIMED_OUT;
			else i =WQ_FAIL;
		}
		return i;
#else
	return WQ_INVALID;
#endif
}
void Platform_criticalsection_unlock(criticalsection *cs)
{
#if defined  (_platform_mingw) || defined(_platform_win)

#elif defined (_platform_linux)
	if(cs) pthread_mutex_unlock(cs);
#else
#endif

}
