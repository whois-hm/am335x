#pragma once

class live5rtspserver : public RTSPServer
{
	/*
 	 	 refernce code 'live555/DynamicRTSPServer/createNewSMS'
	 */
	class live5clientconnection : public RTSPServer::RTSPClientConnection
	{

	public:
		const struct sockaddr_in &get_addr()
		{
			return fClientAddr;
		}
		live5clientconnection(RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr) :
			RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr)
		{
			printf("new client was incoming... (%s)(%d)\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);
		}
		virtual ~live5clientconnection(){}

	    virtual void handleCmd_OPTIONS()
	    {
	    	printf("\"option\" command recv from (%s)\n", inet_ntoa(fClientAddr.sin_addr));
	    	RTSPServer::RTSPClientConnection::handleCmd_OPTIONS();
	    }
	    virtual void handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
	    {
	    	printf("\"describe\" command recv from (%s)\n", inet_ntoa(fClientAddr.sin_addr));
	    	RTSPServer::RTSPClientConnection::handleCmd_DESCRIBE(urlPreSuffix,  urlSuffix, fullRequestStr);
	    }

	};
	class live5clientsession : public RTSPServer::RTSPClientSession
	{
	public:
		live5clientsession(RTSPServer& ourServer, u_int32_t sessionId) :
			RTSPServer::RTSPClientSession(ourServer, sessionId) { }
		virtual ~live5clientsession() { }
	   virtual void handleCmd_SETUP(RTSPClientConnection* ourClientConnection,
					 char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
	   {
		   live5clientconnection *con = (live5clientconnection *)ourClientConnection;
		   printf("\"setup\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
		   RTSPServer::RTSPClientSession::handleCmd_SETUP(ourClientConnection, urlPreSuffix, urlSuffix, fullRequestStr);
	   }
		virtual void handleCmd_TEARDOWN(RTSPClientConnection* ourClientConnection,
						ServerMediaSubsession* subsession)
		{
			live5clientconnection *con = (live5clientconnection *)ourClientConnection;
			printf("\"teardown\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
			RTSPServer::RTSPClientSession::handleCmd_TEARDOWN(ourClientConnection, subsession);
			if(((live5rtspserver *)&fOurRTSPServer)->_report._teardownclient)
			{
				((live5rtspserver *)&fOurRTSPServer)->_report._teardownclient(((live5rtspserver *)&fOurRTSPServer)->_report._ptr,
						std::string(inet_ntoa(con->get_addr().sin_addr)),
						fOurSessionId);
			}


		}
		virtual void handleCmd_PLAY(RTSPClientConnection* ourClientConnection,
					ServerMediaSubsession* subsession, char const* fullRequestStr)
		{
			live5clientconnection *con = (live5clientconnection *)ourClientConnection;
			printf("\"play\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
			RTSPServer::RTSPClientSession::handleCmd_PLAY(ourClientConnection, subsession, fullRequestStr);

			if(((live5rtspserver *)&fOurRTSPServer)->_report._playingclient)
			{
				((live5rtspserver *)&fOurRTSPServer)->_report._playingclient(((live5rtspserver *)&fOurRTSPServer)->_report._ptr,
						std::string(inet_ntoa(con->get_addr().sin_addr)),
						fOurSessionId);
			}


		}
	    virtual void handleCmd_PAUSE(RTSPClientConnection* ourClientConnection,
					 ServerMediaSubsession* subsession)
	    {
	    	live5clientconnection *con = (live5clientconnection *)ourClientConnection;
	    	printf("\"pause\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
	    	RTSPServer::RTSPClientSession::handleCmd_PAUSE(ourClientConnection, subsession);
	    }

	};
	virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
	{
		return new live5clientconnection(*this, clientSocket, clientAddr);
	}

	virtual ClientSession* createNewClientSession(u_int32_t sessionId)
	{
		return new live5clientsession(*this, sessionId);
	}

public:
	enum operation_mode
	{
		none,
		local_file,
		proxy,
		livemediapp
	};
	struct report
	{
		/*new client was reqeust playing*/
		typedef std::function<void (void *ptr, std::string clientip,  unsigned clientsessionid)> playingclient;
		/*client was end of stream*/
		typedef std::function<void (void *ptr, std::string clientip, unsigned clientsessionid)> teardownclient;


		void *_ptr;
		playingclient _playingclient;
		teardownclient _teardownclient;

		/*stream out
		 * warnning .
		 * we notify streamdata is our mediasource's data pointer
		 * you should just reference data pointer
		 * */
		livemediapp_serversession::stream_out_ref _streamout;
		report() :
			_ptr(nullptr),
			_playingclient(nullptr),
			_teardownclient(nullptr),
			_streamout(nullptr){}
		report(void *ptr,
				playingclient &&a,
				teardownclient &&b,
				livemediapp_serversession::stream_out_ref &&c) :
				_ptr(ptr),
				_playingclient(a),
				_teardownclient(b),
				_streamout(c) {}
	};

	live5rtspserver(
			UsageEnvironment& env,
			struct report report,
			char const *url,/* source where*/
			char const *session_name,	/*our stream prefix*/
			char const *proxy_id = nullptr,/*client's authentication*/
			char const *proxy_pwd = nullptr,/*client's authentication*/
			Port ourPort = 554,	/*our server port*/
			char const *usrid = nullptr,/*server's authentication*/
			char const *usrpwd = nullptr,/*server's authentication*/
		    unsigned reclamationSeconds = 0/*infinite service to client*/) :
		    RTSPServer(env,
		    GenericMediaServer::setUpOurSocket(env, ourPort),
			 ourPort,
			 nullptr,
			 reclamationSeconds),
			 _session_name(session_name ? session_name : url),
			 _url(url),
			 _mode(none),
			 _report(report)
	{
		DECLARE_THROW(!_url, "can't parse url");
		DECLARE_THROW(fServerSocket == -1, "can't use socket port");
		if(usrid && usrpwd)
		{
			UserAuthenticationDatabase *authdb = new UserAuthenticationDatabase;
			authdb->addUserRecord(usrid, usrpwd);
			setAuthenticationDatabase(authdb);
		}
		select_url_local_file();
		select_url_proxy(proxy_id, proxy_pwd);
		select_url_livemediapp();
		DECLARE_THROW(_mode == none, "can't selection server mode");

		char *rtspurl = rtspURL(lookupServerMediaSession(_session_name));
		printf("run server : %s\n", rtspurl);
		delete [] rtspurl;
	}
	virtual ~live5rtspserver()
	{

	}
private:
	template <typename _T,
	typename ... Args>
	std::pair<ServerMediaSession *,
	_T *>
	make_session_local_file( char const *info,
			char const *des,
			Args ...args)
	{
		/*
			main session create if null 
		*/
		if(nullptr ==
				lookupServerMediaSession(_session_name))
		{
			this->addServerMediaSession(
				ServerMediaSession::createNew(envir(),
					_session_name,
					info,
					des,
					false,
					nullptr));
		}
		/*
			return the your request typename 'T'subsession
		*/
		return  std::make_pair(
		lookupServerMediaSession(_session_name),
		_T::createNew(envir(), _url, args...));
	}
	void new_session_local_file_aac()
	{
		/*assumed to be an aac audio (adts format)file*/
		std::pair<ServerMediaSession *,
		ADTSAudioFileServerMediaSubsession *> s = 
		make_session_local_file<ADTSAudioFileServerMediaSubsession>("aac audio session",
				"streamed by livemedia++",
				false);

		s.first->addSubsession(s.second);
	}
	void new_session_local_file_amr()
	{
		/* assumed to be an amr audio file*/
		std::pair<ServerMediaSession *,
				AMRAudioFileServerMediaSubsession *> s =
				make_session_local_file<AMRAudioFileServerMediaSubsession>("amr audio session",
						"streamed by livemedia++",
						false);
	   s.first->addSubsession(s.second);
	}
	void new_session_local_file_ac3()
	{
		/*assumed to be an ac3 audio filfe*/
		std::pair<ServerMediaSession *,
		AC3AudioFileServerMediaSubsession *> s =
		make_session_local_file<AC3AudioFileServerMediaSubsession>("ac3 audio session",
				"streamed by livemedia++",
				false);

		s.first->addSubsession(s.second);
	}
	void new_session_local_file_m4e()
	{
		/*assumed to be a mpeg-4 video elementary stream file*/
		std::pair<ServerMediaSession *,
		MPEG4VideoFileServerMediaSubsession *> s =
		make_session_local_file<MPEG4VideoFileServerMediaSubsession>("m4e video session",
				"streamed by livemedia++",
				false);
		s.first->addSubsession(s.second);
	}
	void new_session_local_file_264()
	{
		/*assumed to be a h264 video elementary streamfile*/
		std::pair<ServerMediaSession *,
		H264VideoFileServerMediaSubsession *> s = 
		make_session_local_file<H264VideoFileServerMediaSubsession>("264 video session",
				"streamed by livemedia++",
				false);

		OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
		s.first->addSubsession(s.second);
	}
	void new_session_local_file_265()
	{
		/*assumed to be a h265 video elementary streamfile*/
		std::pair<ServerMediaSession *,
		H265VideoFileServerMediaSubsession *> s = 
		make_session_local_file<H265VideoFileServerMediaSubsession>("265 video session",
				"streamed by livemedia++",
				false);

		OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.265 frames
		s.first->addSubsession(s.second);
	}
	void new_session_local_file_mp3()
	{
		/*assumed to be a mpeg-1 or 2 audio file*/
	    // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
	//#define STREAM_USING_ADUS 1
	    // To also reorder ADUs before streaming, uncomment the following:
	//#define INTERLEAVE_ADUS 1
	    // (For more information about ADUs and interleaving,
	    //  see <http://www.live555.com/rtp-mp3/>)
		std::pair<ServerMediaSession *,
		MP3AudioFileServerMediaSubsession *> s =
		make_session_local_file<MP3AudioFileServerMediaSubsession>("mp3 audio session",
				"streamed by livemedia++",
				false,
				false,/*stream using adus*/
				nullptr/*interleaved adus*/
				);


		s.first->addSubsession(s.second);
	}
	void new_session_local_file_mpg()
	{
		/*assumed to be a mpeg-1 or 2 program stream(audio+video)file*/
		std::pair<ServerMediaSession *,
		MPEG1or2FileServerDemux *> s = 
		make_session_local_file<MPEG1or2FileServerDemux>("mpg video/audio session",
				"streamed by livemedia++",
				false);

		s.first->addSubsession(s.second->newVideoServerMediaSubsession());
		s.first->addSubsession(s.second->newAudioServerMediaSubsession());
	}
	void new_session_local_file_vob()
	{
		/*assumed to be a vob (mpeg-2 program stream, with ac -3 audio) file*/
		std::pair<ServerMediaSession *,
				MPEG1or2FileServerDemux *> s = 
				make_session_local_file<MPEG1or2FileServerDemux>("vob video/audio session",
						"streamed by livemedia++",
						false);

		s.first->addSubsession(s.second->newVideoServerMediaSubsession());
		s.first->addSubsession(s.second->newAC3AudioServerMediaSubsession());
	}
	void new_session_local_file_ts()
	{
		/*assumed to be a mpeg trnsport stream file:
		 * use an index file name that's the same as the ts file name, except with .tsx*/

		unsigned indexfilenamelen = strlen(_url) + 2;
		char *indexfilename = new char [indexfilenamelen];
		sprintf(indexfilename, "%sx", _url);


		std::pair<ServerMediaSession *,
		MPEG2TransportFileServerMediaSubsession *> s =
		make_session_local_file<MPEG2TransportFileServerMediaSubsession>("ts session",
				"streamed by livemedia++",
				indexfilename,
				false);

		s.first->addSubsession(s.second);
		delete [] indexfilename;
	}
	void new_session_local_file_wav()
	{
		// To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
	    // change the following to True:
	    Boolean convertToULaw = False;

		/*assumed to be a wav audio file*/
	    std::pair<ServerMediaSession *,
		WAVAudioFileServerMediaSubsession *> s = 
		make_session_local_file<WAVAudioFileServerMediaSubsession>("wav audio session",
				"streamed by livemedia++",
				false,
				convertToULaw);

	    s.first->addSubsession(s.second);
	}
	void new_session_local_file_dv()
	{
		/*assumed to be a dv video file
		 * first, make sure that the rtpsinks buffers will be large enough to handle the huge size of dv frames (as big as 288000).*/
		OutPacketBuffer::maxSize = 300000;
		std::pair<ServerMediaSession *,
		DVVideoFileServerMediaSubsession *> s = 
		make_session_local_file<DVVideoFileServerMediaSubsession>("dv session",
				"streamed by livemedia++",
				false);

		s.first->addSubsession(s.second);
	}
	void new_session_proxy(char const *proxy_id, char const *proxy_pwd)
	{
	    addServerMediaSession(
	    ProxyServerMediaSession::createNew(envir(),
	    		  this,
				  _url,
				  _session_name,
				  proxy_id,
				  proxy_pwd,
				  0,
				  10));
	}
	void new_session_livemediapp()
	{
		/*
			main session create if null 
		*/
		if(nullptr ==
				lookupServerMediaSession(_session_name))
		{
			ServerMediaSession *livemediappsession = nullptr;
			livemediappsession = livemediapp_serversession::createnew(_url,
					envir(),
					_session_name,
					"event session",
					"streamd by livemedia++",
					false,
					nullptr);
			DECLARE_THROW(!livemediappsession, "can't create livemedia pp serversession..");

			if(_report._streamout)
			{
				((livemediapp_serversession *)livemediappsession)->start_streamout_filter(_report._streamout);
			}
			this-> addServerMediaSession(livemediappsession);
		}
	}
		
	void select_url_local_file()
	{
		if(_mode != none)
		{
			return;
		}

		struct
		{
			char const *ext;
			void (live5rtspserver::*f)();
		}table[] =
		{
				/*this live555 support class*/
				{".aac", 	&live5rtspserver::new_session_local_file_aac},
				{".amr", 	&live5rtspserver::new_session_local_file_amr},
				{".ac3", 	&live5rtspserver::new_session_local_file_ac3},
				{".m4e", 	&live5rtspserver::new_session_local_file_m4e},
				{".264", 	&live5rtspserver::new_session_local_file_264},
				{".265",	&live5rtspserver::new_session_local_file_265},
				{".mp3", 	&live5rtspserver::new_session_local_file_mp3},
				{".mpg",  	&live5rtspserver::new_session_local_file_mpg},
				{".vob", 	&live5rtspserver::new_session_local_file_vob},
				{".ts", 	&live5rtspserver::new_session_local_file_ts},
				{".wav",  	&live5rtspserver::new_session_local_file_wav},
				{".dv",		&live5rtspserver::new_session_local_file_dv},
		};

		for(int i = 0; i < DIM(table); i++)
		{
			if(contain_string(_url, table[i].ext))
			{
				(this->*table[i].f)();
				_mode = local_file;
				break;
			}
		}
	}
	void select_url_proxy(char const *proxy_id, char const *proxy_pwd)
	{
		if(_mode != none)
		{
			return;
		}

		if(!contain_string(_url, "rtsp"))
		{
			return ;
		}

		new_session_proxy(proxy_id, proxy_pwd);
		_mode = proxy;
	}
	void select_url_livemediapp()
	{
		if(_mode != none)
		{
			return;
		}

		if(contain_string(_url, "/dev") ||
				contain_string(_url, ".mp4"))
		{
			new_session_livemediapp();
			_mode = livemediapp;
		}
	}

private:
	char const *_session_name;
	char const *_url;
	enum operation_mode _mode;
	struct report _report;
};
