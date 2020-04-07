#pragma once


class mediaserver :
		public live5scheduler
		<live5rtspserver>
{
public:
	mediaserver() :
		live5scheduler <live5rtspserver>(){ }
	virtual ~mediaserver() { }

	int start(bool atother,
			live5rtspserver::report report,
			char const *url,
			char const*sessionname,
			char const*proxy_id,
			char const*proxy_pwd,
			int port,
			char const*server_id = nullptr,
			char const *server_pwd = nullptr)
	{
		if(url)_attr.set("url", url, 0, 0.0);
		if(sessionname)_attr.set("session name", sessionname, 0, 0.0);
		if(proxy_id)_attr.set("proxy id", proxy_id, 0, 0.0);
		if(proxy_pwd)_attr.set("proxy pwd", proxy_pwd, 0, 0.0);
		if(server_id)_attr.set("server id", server_id, 0, 0.0);
		if(server_pwd)_attr.set("server pwd", server_pwd, port, 0.0);
		_attr.set("port", "port", port, 0.0);
		
		if(!can())
		{
			return -1;
		}

		live5scheduler<live5rtspserver>::start(atother,
						report,
						_attr.get_char("url"),
						_attr.get_char("session name"),
						_attr.get_char("proxy id"),
						_attr.get_char("proxy pwd"),
						_attr.get_int("port"),
						_attr.get_char("server id"),
						_attr.get_char("server pwd"));
		return 0;
	}
private:
	bool can()
	{
		return (!_attr.notfound("url") ||
				!_attr.notfound("port")) &&
				!_attr.notfound("session name");

	}
	avattr _attr;
};
