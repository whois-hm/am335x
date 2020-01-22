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
#define __base__malloc__  libwq_malloc

typedef std::function<void (void *)> base_deallocator;
#define __base__free__ libwq_free





typedef std::lock_guard<std::mutex> autolock;

inline
unsigned  sys_time_c()
{
	struct timeval v;
	gettimeofday(&v, NULL);

	return (v.tv_sec * 1000) + (v.tv_usec / 1000);
}
