#include "_WQ.h"
WQ_API int CS_open(criticalsection *cs)
{
	return Platform_criticalsection_open(cs);
}
WQ_API _dword CS_lock(criticalsection *cs, _dword time)
{
	return Platform_criticalsection_lock(cs, time);
}
WQ_API void CS_unlock(criticalsection *cs)
{
	return Platform_criticalsection_unlock(cs);
}
WQ_API void CS_close(criticalsection *cs)
{
	return Platform_criticalsection_close(cs);
}
