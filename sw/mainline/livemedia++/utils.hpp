#pragma once

typedef std::tuple
		<
		int,/*hour*/
		int,/*min*/
		int,/*sec*/
		int,/*ms*/
		int64_t/*duration given ffmpeg*/
		>
duration_div;

/*
 	 	 use exam this,
 	 	 case video : width, height, format
 	 	 case audio : channel, samplerate, format
 */
typedef std::tuple
		<
		int,
		int,
		int
		>
triple_int;

/*
 	 use exam this,
 	 first triple : origin video or audio
 	 second triple : new video or audio
 */
typedef std::pair
		<
		triple_int,
		triple_int
		>
compare_triple_int;

class pcm;
typedef std::pair<pcm,/*pcm data*/
		int/*requre pcm size*/
		> pcm_require;

typedef std::function<void * (size_t)> base_allocator;
#define __base__malloc__ malloc// libwq_malloc

typedef std::function<void (void *)> base_deallocator;
#define __base__free__ free//libwq_free

/*
inline void* operator new(std::size_t size)
{
	void *userblock = __base__malloc__(size);
	return  userblock;
}
inline void* operator new[](std::size_t size)
{
	void *userblock = __base__malloc__(size);
	return  userblock;
}
inline void operator delete(void *ptr)
{
	if(ptr) __base__free__(ptr);
}
inline void operator delete[](void *ptr)
{ if(ptr) __base__free__(ptr); }
*/



typedef std::lock_guard<std::mutex> autolock;
inline bool contain_string(char const *str, char const *findstr)
{
	std::string dump(str);

	if(dump.empty() ||
			!findstr)
	{
		return false;
	}
	return dump.find(findstr) != std::string::npos;
}

inline
unsigned  sys_time_c()
{
	struct timeval v;
	gettimeofday(&v, NULL);

	return (v.tv_sec * 1000) + (v.tv_usec / 1000);
}
