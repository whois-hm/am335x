#define WQ_API_EXPORT
#include "WQ.h"



#define q_iNext(cursor, max)	(	((cursor + 1) >= max) ? (0) : (cursor  + 1))
#define q_iPrev(cursor, max)	((cursor == 0) ? (max - 1) : (cursor -1))

#define BIT_SET(byte_dst, bit)					( byte_dst |= bit )
#define BIT_CLEAR(byte_dst, bit)			( byte_dst &= (~bit))
#define IS_BIT_SET(byte_dst, bit)			( ( byte_dst & bit ))

#define ELEMENT_FLAGS_SEMA_ACK 	(1 << 0)
#define ELEMENT_FLAGS_SIGNALED		(1 << 1)
struct element
{
	unsigned char flags;
	semaphore ack;
	void *par;
	_dword npar;
};
struct WQ
{
	threadid t_id;
	_dword element_1_size;
	_dword element_length;
	struct
	{
		semaphore reader;
		semaphore writer;
		criticalsection q_sync;
	}syncronize;
	_dword head_cursor;
	_dword tail_cursor;
	struct element *work_elements;
};

struct BackGround
{
	threadid t_id;
	BackGround_function func;
	void *p;
};
struct Keyvalue
{
	struct Keyvalue *next;
	char *k;
	char *vc;
	int vi;
};
struct Dictionary
{
	struct Keyvalue *kvs;
};

int Platform_thread_open(struct BackGround *b);
void Platform_thread_close(struct BackGround *b);
void  Platform_sleep(_dword time);
_dword Platform_tick_count();
void  Platform_thread_id(threadid *id);
int Platform_semephore_open(semaphore *sema, _dword init, _dword max);
void  Platform_semaphore_close(semaphore *sema);
_dword Platform_semaphore_lock(semaphore *sema, _dword time);
void Platform_semaphore_unlock(semaphore *sema);
int Platform_criticalsection_open(criticalsection *cs);
void  Platform_criticalsection_close(criticalsection *cs);
_dword Platform_criticalsection_lock(criticalsection *cs, _dword time);
void Platform_criticalsection_unlock(criticalsection *cs);
