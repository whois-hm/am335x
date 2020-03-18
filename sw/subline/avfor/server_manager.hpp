#pragma once
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

		_server = new mediaserver();
		return 0 == _server->start(true, 
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
