#include "_WQ.h"

static int alloc_wq(_dword size, _dword length, struct WQ **out)
{
	if(size <= 0 ||
			length <= 0 ||
			!out)
	{
		return WQEINVAL;
	}
	(*out) = (struct WQ *)libwq_malloc(sizeof(struct WQ));
	if(!(*out))
	{
		return WQENOMEM;
	}
	 memset((*out), 0, sizeof(struct WQ));

	 (*out)->element_1_size = size;
	 (*out)->element_length = length;
	 Platform_thread_id(&(*out)->t_id);

	 return WQOK;
}
static int syncs_create(struct WQ *wq)
{
	if(wq)
	{
		if(Platform_criticalsection_open(&wq->syncronize.q_sync) == WQOK)
		{
			if(Platform_semephore_open(&wq->syncronize.reader, 0, 1) == WQOK)
			{
				if(Platform_semephore_open(&wq->syncronize.writer, wq->element_length, wq->element_length) == WQOK)
				{
					return WQOK;
				}
			}
		}
	}

	return WQENODEV;
}
static int elements_create(struct WQ *wq)
{
	_dword n = 0;
	struct element *a_elements = NULL;

	wq->work_elements = (struct element *)libwq_malloc(sizeof(struct element) * wq->element_length);
	if(!wq->work_elements)
	{
		return WQENOMEM;
	}
	for(; n < wq->element_length; n++)
	{
		a_elements = wq->work_elements + n;
		a_elements->par = libwq_malloc(wq->element_1_size);
		if(!a_elements->par)
		{
			return WQENOMEM;
		}
		if(Platform_semephore_open(&a_elements->ack, 0, 1) != WQOK)
		{
			return WQENODEV;
		}
	}
	return WQOK;
}
static void resource_free(struct WQ *wq)
{
	_dword n = 0;
	struct element *f_elements = NULL;

	if(wq)
	{
		if(wq->work_elements)
		{
			for(; n < wq->element_length; n++)
			{
				f_elements = wq->work_elements + n;
				Platform_semaphore_close(&f_elements->ack);
				if(f_elements->par)
				{
					libwq_free(f_elements->par);
					f_elements->par = NULL;
				}
			}
			libwq_free(wq->work_elements);
			wq->work_elements = NULL;
		}

		Platform_criticalsection_close(&wq->syncronize.q_sync);
		Platform_semaphore_close(&wq->syncronize.reader);
		Platform_semaphore_close(&wq->syncronize.writer);

		libwq_free(wq);
		wq = NULL;
	}
}
static void threshold(struct WQ *wq)
{
	void *par = NULL;
	_dword npar = 0;
	_dword threshold_time = 20;

	while(WQ_SIGNALED == WQ_recv(wq, &par, &npar, threshold_time))
	{
		/*
		  empty!
		 */
	}
}
WQ_API struct WQ *WQ_open(_dword size, _dword length)
{
	struct WQ *wq = NULL;

	if((alloc_wq(size, length, &wq) == WQOK) &&
			(syncs_create(wq) == WQOK) &&
			(elements_create(wq) == WQOK))
	{
		return wq;
	}
	resource_free(wq);
	return NULL;
}
WQ_API void WQ_close(struct WQ **pwq)
{
	struct WQ *wq = NULL;
	if(!pwq  ||
			!(*pwq))
	{
		return;
	}
	wq = *pwq;
	*pwq = NULL;

	threshold(wq);
	resource_free(wq);
}

WQ_API _dword WQ_recv(struct WQ *wq, void **ppar, _dword *pnpar, _dword time)
{
	_dword ret = WQ_FAIL;

	if(!wq)
	{
		return WQEINVAL;
	}

	Platform_criticalsection_lock(&wq->syncronize.q_sync, INFINITE);
	if(!IS_BIT_SET(wq->work_elements[wq->head_cursor].flags, ELEMENT_FLAGS_SIGNALED))
	{
		Platform_criticalsection_unlock(&wq->syncronize.q_sync);
		goto recv;
	}
	if(IS_BIT_SET(wq->work_elements[wq->head_cursor].flags, ELEMENT_FLAGS_SEMA_ACK))
	{
		Platform_semaphore_unlock(&wq->work_elements[wq->head_cursor].ack);
		BIT_CLEAR(wq->work_elements[wq->head_cursor].flags, ELEMENT_FLAGS_SEMA_ACK);
	}

	BIT_CLEAR(wq->work_elements[wq->head_cursor].flags, ELEMENT_FLAGS_SIGNALED);
	wq->head_cursor = q_iNext(wq->head_cursor, wq->element_length);

	Platform_criticalsection_unlock(&wq->syncronize.q_sync);
	Platform_semaphore_unlock(&wq->syncronize.writer);

recv:
	if((ret = Platform_semaphore_lock(&wq->syncronize.reader, time)) != WQ_SIGNALED)
	{
		return ret;
	}
	Platform_criticalsection_lock(&wq->syncronize.q_sync, INFINITE);
	if(ppar)
	{
		*ppar = wq->work_elements[wq->head_cursor].par;
	}
	if(pnpar)
	{
		*pnpar = wq->work_elements[wq->head_cursor].npar;
	}
	BIT_SET(wq->work_elements[wq->head_cursor].flags, ELEMENT_FLAGS_SIGNALED);
	Platform_criticalsection_unlock(&wq->syncronize.q_sync);
	return WQ_SIGNALED;

}
WQ_API _dword WQ_send(struct WQ *wq, void *par, _dword npar, _dword time, int flags)
{
	semaphore *ack_sema = NULL;
	_dword ret = WQ_FAIL;
		if(!wq ||
				!par ||
				npar <= 0)
		{
			return WQEINVAL;
		}

		if((ret = Platform_semaphore_lock(&wq->syncronize.writer, time)) != WQ_SIGNALED)
		{
			return ret;
		}

		Platform_criticalsection_lock(&wq->syncronize.q_sync, INFINITE);

		memset((void *)wq->work_elements[wq->tail_cursor].par, 0, wq->element_1_size);
		memcpy((void *)wq->work_elements[wq->tail_cursor].par, par, npar);


		wq->work_elements[wq->tail_cursor].npar = npar;

		if(IS_BIT_SET(flags, WQ_SEND_FLAGS_BLOCK))
		{
			BIT_SET(wq->work_elements[wq->tail_cursor].flags, ELEMENT_FLAGS_SEMA_ACK);
			ack_sema = &wq->work_elements[wq->tail_cursor].ack;
		}
		wq->tail_cursor = q_iNext(wq->tail_cursor, wq->element_length);;

		Platform_criticalsection_unlock(&wq->syncronize.q_sync);
		Platform_semaphore_unlock(&wq->syncronize.reader);

		if(ack_sema)
		{
			Platform_semaphore_lock(ack_sema, INFINITE);
		}
		return WQ_SIGNALED;
}
