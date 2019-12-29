#include "core.hpp"

/*
 	 our singleton instance
 */

livemedia_pp *livemedia_pp::obj = nullptr;


livemedia_pp*livemedia_pp::ref()
{

	if(!obj)
	{
#if defined libwq_heap_testmode
	libwq_heap_testinit();
#endif
		obj = new livemedia_pp();
	}

	return obj;
}
/*
 	 delete our instance
 */
void livemedia_pp::unref()
{
	if(obj)
	{
		delete obj;
#if defined libwq_heap_testmode
	libwq_heap_testdeinit();
#endif
	}
	obj = nullptr;

}


playback_inst * playback_inst::create_new(const avattr &attr,
		char const *name,
		unsigned time,
		char const *id,
		char const *pwd)
{
	playback_inst *inst = nullptr;
	if(!attr.has_frame_any())
	{
		return nullptr;
	}
	if(!access(name, 0))
	{
		inst = new local_playback(attr, name);
	}
#if defined have_live555
	if(!inst)
	{
		inst = new rtsp_playback(attr, name, time, id, pwd);
	}
#endif
	if(inst)
	{
		inst->connectionwait();
	}
	return inst;
}

