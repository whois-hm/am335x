#include "_WQ.h"
WQ_API int SEMA_open(semaphore *sem, _dword i, _dword m)
{
	return Platform_semephore_open(sem, i, m);
}
WQ_API _dword SEMA_lock(semaphore *sem, _dword time)
{
	return Platform_semaphore_lock(sem, time);
}
WQ_API void SEMA_unlock(semaphore *sem)
{
	return Platform_semaphore_unlock(sem);
}
WQ_API void SEMA_close(semaphore *sem)
{
	return Platform_semaphore_close(sem);
}
