#include "_WQ.h"

WQ_API struct BackGround *BackGround_turn(BackGround_function fn, void *p)
{
	struct BackGround *background = NULL;
	int ret = 0;
	if(!fn)
	{
		return NULL;
	}
	background = (struct BackGround *)libwq_malloc(sizeof(struct BackGround));
	if(background)
	{
		background->func = fn;
		background->p = p;
	}
	ret = Platform_thread_open(background);
	if(ret != WQOK)
	{
		if(background)
		{
			libwq_free(background);
		}
		return NULL;
	}
	return background;
}
WQ_API void BackGround_turnoff(struct BackGround **pb)
{
	if(!pb ||
			!(*pb))
	{
		return;
	}
	struct BackGround *background = (*pb);
	*pb = NULL;

	Platform_thread_close(background);
	libwq_free(background);
}
WQ_API threadid BackGround_id()
{
	threadid id;
	Platform_thread_id(&id);
	return id;
}
