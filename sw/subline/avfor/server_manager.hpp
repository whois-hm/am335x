#pragma once
struct server_manager_incomclient_par
{
	std::string _clientip;
	unsigned _clientsessionid;
	server_manager_incomclient_par(std::string clientip,
		unsigned clientsessionid) :
			_clientip(clientip),
			_clientsessionid(clientsessionid){}
};
struct server_manager_teardownclient_par
{
	std::string _clientip;
		unsigned _clientsessionid;
		server_manager_teardownclient_par(std::string clientip,
			unsigned clientsessionid) :
				_clientip(clientip),
				_clientsessionid(clientsessionid){}
};
struct server_manager_streamoutref_par
{
	unsigned _clientsessionid;
	enum AVMediaType _mediatype;
	enum AVCodecID _codecid;
	//unsigned char *streamdata,
	unsigned _streamdata_size;
	server_manager_streamoutref_par(	unsigned clientsessionid,
	enum AVMediaType mediatype,
	enum AVCodecID codecid,
	//unsigned char *streamdata,
	unsigned streamdata_size) :
		_clientsessionid(clientsessionid),
		_mediatype(mediatype),
		_codecid(codecid),
		//unsigned char *streamdata,
		_streamdata_size(streamdata_size){}

};
class server_manager : public manager
{
private:
	mediaserver * _server;
public:
	server_manager(avfor_context  *avc,
			ui *interface) : 
			manager(avc, interface),
				_server(nullptr)
	{

	}
	virtual ~server_manager()
	{
		if(_server)
		{
			delete _server;
			_server = nullptr;
		}
	}
	virtual bool ready()
	{

		std::string source_url, sessionname, proxy_id, proxy_pwd, serverid, serverpwd;
		int port = 0;
		_avc->get(section_server, url,  source_url);
		_avc->get(section_server, server_session_name,  sessionname);
		_avc->get(section_server, proxy_autentication_id,  proxy_id);
		_avc->get(section_server, proxy_autentication_password, proxy_pwd);
		_avc->get(section_server, server_autentication_id,  serverid);
		_avc->get(section_server, server_autentication_password,  serverpwd);
		_avc->get(section_server, server_port,  port);

		live5rtspserver::report _report;
//		live5rtspserver::report _report(
//						nullptr,
//						[&](void *ptr, std::string clientip,  unsigned clientsessionid)->void
//						{
//							struct pe_user u;
//							u._code = custom_code_section_server_incoming_client;
//							u._ptr = (void *)new server_manager_incomclient_par(clientip, clientsessionid);
//							_int->write_user(u);
//						},
//						[&](void *ptr, std::string clientip, unsigned clientsessionid)->void
//						{
//							struct pe_user u;
//							u._code = custom_code_section_server_teardown_client;
//							u._ptr = (void *)new server_manager_teardownclient_par(clientip, clientsessionid);
//							_int->write_user(u);
//						},
//						[&](unsigned clientsessionid,
//								enum AVMediaType mediatype,
//								enum AVCodecID codecid,
//								unsigned char *streamdata,
//								unsigned streamdata_size)->void
//						{
//							struct pe_user u;
//							u._code = custom_code_section_server_streamout_reference;
//							u._ptr = (void *)new server_manager_streamoutref_par(clientsessionid,
//									mediatype,
//									codecid,
//									streamdata_size);
//							_int->write_user(u);
//						}
//		);

		_server = new mediaserver();
		return 0 == _server->start(true, 
				_report,
			source_url.empty() ? nullptr : source_url.c_str(),
			sessionname.empty() ? nullptr : sessionname.c_str(),
			proxy_id.empty() ? nullptr : proxy_id.c_str(),
			proxy_pwd.empty() ? nullptr : proxy_pwd.c_str(),
			port,
			serverid.empty() ? nullptr : serverid.c_str(),
			serverpwd.empty() ? nullptr : serverpwd.c_str());

	}
	virtual void operator()(ui_event &e)
	{
	}	
};
