#pragma once


#if defined __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
extern "C"
{

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)

#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libpostproc/postprocess.h>
}



#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"
#include "Groupsock.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"



#include "WQ.h"

#define LOCATION_STEMP	"> [File : %s] [Function : %s] [Line : %d]"
#define LOCATION_FILE	(__FILE__)
#define LOCATION_FUNC	(	__FUNCTION__  )
#define LOCATION_LINE	(	  __LINE__    )
#define log_at()	printf(LOCATION_STEMP"\r\n", LOCATION_FILE, LOCATION_FUNC, LOCATION_LINE);

#include <type_traits>
#include <iostream>
#include <execinfo.h>
#include <stdexcept>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>
#include <tuple>
#include <mutex>
#include <atomic>
#include <locale>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <functional>
#include "utils.hpp"
#include "livemedia++.hpp"
#include "wthread.hpp"
#include "filter.hpp"
#include "busyscheduler.hpp"
#include "avattr.hpp"
#include "media_data.hpp"
#include "avpacket_class.hpp"
#include "mediacontainer.hpp"
#include "avpacket_bsf.hpp"
#include "avframe_class.hpp"
#include "pixel.hpp"
#include "pcm.hpp"
#include "pixelframe.hpp"
#include "pcmframe.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "swxcontext_class.hpp"
#include "mediacontainer_record.hpp"
#include "pixelutils.hpp"
#include "framescheduler.hpp"
#include "uvc.hpp"
#include "live5scheduler.hpp"
#include "live5livemediapp_sessions.hpp"
#include "live5rtspserver.hpp"
#include "live5rtspclient.hpp"
#include "mediaserver.hpp"
#include "playback_inst.hpp"
#include "local_playback.hpp"
#include "rtsp_playback.hpp"
#include "uvc_playback.hpp"
#include "playback.hpp"
#include "ui.hpp"


