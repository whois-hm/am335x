#if defined (__MINGW64__) || defined (__MINGW32__)
#define _platform_mingw
#if defined (__MINGW64__)
#define _platform_mingw64
#else
#define _platform_mingw32
#endif
#elif defined (_WIN32 ) || defined (_WIN64)
#define _platform_win
#if defined (_WIN32)
#define _platform_win32
#else
#define _platform_win64
#endif
#else
#if defined (__GNUC__)
#define _platform_linux
#if defined (__x86_64__) || defined(__ppc64__)
#define _platform_linux64
#else
#define _platform_linux32
#endif
#endif
#endif

#if defined (__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#if defined (_platform_mingw) || defined(_platform_win)
#if defined (WQ_API_EXPORT)
#define WQ_API  EXTERN_C __declspec(dllexport)
#else
#define WQ_API EXTERN_C __declspec(dllimport)
#endif
#else
#if defined (WQ_API_EXPORT)
#define WQ_API
#else
#define WQ_API EXTERN_C
#endif
#endif

#define libwq_heap_testmode
#include  <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#if defined (_platform_linux)
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>
#include <mqueue.h>
#include  <execinfo.h>
#include  <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include  <sys/vtimes.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/videodev2.h>
#include <sys/shm.h>
#include <sys/un.h>
#elif defined  (_platform_mingw) || defined(_platform_win32)
#endif

typedef unsigned short _word;
typedef unsigned long _dword;

#if defined  (_platform_mingw) || defined(_platform_win32)
typedef HANDLE semaphore;
typedef HANDLE criticalsection;
typedef _dword	threadid;
#elif defined (_platform_linux)
typedef sem_t semaphore;
typedef pthread_mutex_t criticalsection;
typedef pthread_t	threadid;
#endif


#if !defined (NULL)
#define NULL ((void *)0)
#endif
#if !defined (INFINITE)
#define INFINITE	(_dword)0xffffffffL
#endif

#if defined (_platform_linux)
#define WAIT_OBJECT_0				(_dword)0x00000000L
#define WAIT_FAILED					(_dword)0xffffffffL
#define WAIT_TIMEOUT				(_dword)0x00000102L
#define WAIT_ABANDONED		(_dword)0x00000080L
#endif

#define WQ_SIGNALED 		WAIT_OBJECT_0
#define WQ_FAIL						WAIT_FAILED
#define WQ_TIMED_OUT 		WAIT_TIMEOUT
#define WQ_INVALID 			WAIT_ABANDONED

#define SQUARE(x) 											((x) * (x))
#define BIT_SET(byte_dst, bit)					( byte_dst |= bit )
#define BIT_CLEAR(byte_dst, bit)			( byte_dst &= (~bit))
#define IS_BIT_SET(byte_dst, bit)			( ( byte_dst & bit ))
#define DIM(x) 													((int)(sizeof(x) / sizeof(x[0])))
#define X_ABS(x)        										( ((x)<0)?-(x):(x) )

/*Success*/
#define WQOK					 0
/*Not Enough Memory*/
#define WQENOMEM 		1
/*Invalid Parameter*/
#define WQEINVAL			2
/*No Such Device*/
#define WQENODEV		3
/*Resource Busy*/
#define WQEBUSY			4
/*Bad Descriptor*/
#define WQEBADF			5
/*Connection Timeout*/
#define WQETIMEDOUT 6
/*Already Use*/
#define WQEALREADY 7

#define WQ_SEND_FLAGS_BLOCK	(1 << 0)




struct WQ;
struct BackGround;
struct Dictionary;
typedef void (*BackGround_function)(void *p);


WQ_API struct WQ *WQ_open(_dword size, _dword length);
WQ_API void WQ_close(struct WQ **pwq);
WQ_API _dword WQ_recv(struct WQ *wq, void **ppar, _dword *pnpar, _dword time);
WQ_API _dword WQ_send(struct WQ *wq, void *par, _dword npar, _dword time, int flags);

WQ_API struct BackGround *BackGround_turn(BackGround_function fn, void *p);
WQ_API void BackGround_turnoff(struct BackGround **pb);
WQ_API threadid BackGround_id();
WQ_API void Printf_memory(void *mem, _dword size);

WQ_API _dword Time_tickcount();
WQ_API void Time_sleep(_dword time);

WQ_API struct Dictionary *Dictionary_create();
WQ_API void Dictionary_delete(struct Dictionary **pd);
WQ_API _dword Dictionary_add_char(struct Dictionary *d, char *k, char *v);
WQ_API _dword Dictionary_add_int(struct Dictionary *d, char *k, int v);
WQ_API void Dictionary_remove(struct Dictionary *d, char *k);
WQ_API char *Dictionary_refchar(struct Dictionary *d, char *k);
WQ_API char *Dictionary_allocchar(struct Dictionary *d, char *k);
WQ_API void Dictionary_freechar(char **allocchar);
WQ_API int Dictionary_copyint(struct Dictionary *d, char *k);
WQ_API int Dictionary_haskey(struct Dictionary *d, char *k);

WQ_API int CS_open(criticalsection *cs);
WQ_API _dword CS_lock(criticalsection *cs, _dword time);
WQ_API void CS_unlock(criticalsection *cs);
WQ_API void CS_close(criticalsection *cs);

WQ_API int SEMA_open(semaphore *sem, _dword i, _dword m);
WQ_API _dword SEMA_lock(semaphore *sem, _dword time);
WQ_API void SEMA_unlock(semaphore *sem);
WQ_API void SEMA_close(semaphore *sem);


WQ_API void libwq_heap_testinit();
WQ_API void libwq_heap_testdeinit();
WQ_API void *libwq_malloc(size_t size);
WQ_API void libwq_free(void *mem);
WQ_API void libwq_print_heap();

