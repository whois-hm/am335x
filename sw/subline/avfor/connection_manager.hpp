#pragma once
class fdmon
{
#define FD_IN	(1 << 0)
#define FD_OUT	(1 << 1)
#define FD_ERR	(1 << 2)

private:
	struct timeval tv;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	int maxfd;

public:
 	fdmon(){clear();}
	virtual ~fdmon(){clear();}
	void clear()
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		maxfd = -1;
	}
	void set(int fd, _dword flag = (FD_IN | FD_ERR))
	{
		if(fd == -1)
		{
			return;
		}
		if(IS_BIT_SET(flag, FD_IN))
		{
			FD_SET(fd, &readfds);
		}
		if(IS_BIT_SET(flag, FD_OUT))
		{
			FD_SET(fd, &writefds);
		}
		if(IS_BIT_SET(flag, FD_ERR))
		{
			FD_SET(fd, &exceptfds);
		}
		if(maxfd <= fd)
		{
			maxfd = fd;
		}
	}
	int sigwait(int timeout)
	{
		if(maxfd == -1)
		{
			return -1;
		}
		struct timeval *tm = NULL;
		if(timeout >= 0)
		{
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = (timeout % 1000) * 1000;
			tm = &tv;
		}

		return select(maxfd + 1, &readfds, &writefds, &exceptfds, tm);
	}
	_dword issetbit(int fd, _dword flag)
	{
		_dword returnbit = 0;
		if(fd == -1)
		{
			return returnbit;
		}
		if(IS_BIT_SET(flag, FD_IN))
		{
			if(FD_ISSET(fd, &readfds))
			{
				BIT_SET(returnbit, FD_IN);
			}
		}
		if(IS_BIT_SET(flag, FD_OUT))
		{
			if(FD_ISSET(fd, &writefds))
			{
				BIT_SET(returnbit, FD_OUT);
			}
		}
		if(IS_BIT_SET(flag, FD_ERR))
		{
			if(FD_ISSET(fd, &exceptfds))
			{
				BIT_SET(returnbit, FD_ERR);
			}
		}
		return returnbit;
	}
	bool issetbit_err(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_ERR) & FD_ERR;
	}
	bool issetbit_in(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_IN) & FD_IN;
	}
	bool issetbit_out(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_OUT) & FD_OUT;
	}
};

class connection_manager : public manager
{
private:
	std::thread *_th;
	int _socket;
	int _pipe[2];
	void close_fds()
	{
		if(_socket)
		{
			close(_socket);
			_socket = -1;
		}
		if(_pipe[0] != -1)
		{
			close(_pipe[0]);
			_pipe[0] = -1;
		}
		if(_pipe[1] != -1)
		{
			close(_pipe[1]);
			_pipe[1] = -1;
		}
	}
	bool connection_once()
	{
		std::string ip;
		int port = 0;
		int opt = 0;
		struct sockaddr_in addr;

		int ntry = 5;
		_avc->get(section_connection, connection_ip, ip);
		_avc->get(section_connection, connection_port, port);

		printf("socket avfor_server connection to ... %s %d\n", ip.c_str(), port);
		memset( &addr, 0, sizeof( struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr= inet_addr( ip.c_str() );
		do
		{
			close_fds();
			_socket = socket(PF_INET, SOCK_STREAM, 0);
			if(_socket == -1)
			{
				printf("socket create fail\n");
				break;
			}
			opt = fcntl(_socket, F_GETFL);
			if(opt < 0)
			{
				printf("socket can't fcntl F_GETFL\n");
				break;
			}
			opt |= (O_NONBLOCK | O_RDWR);
			if(fcntl(_socket, F_SETFL, opt) < 0)
			{
				printf("socket can't fcntl F_SETFL\n");
				break;
			}
			if(-1 == connect(_socket, (struct sockaddr *)& addr, sizeof(addr)))
			{
				if((errno != EINPROGRESS &&
						errno != EAGAIN))
				{
					break;
				}
				fdmon m;
				m.set(_socket,(FD_IN | FD_OUT | FD_ERR));
				int ret = m.sigwait(1000);
				if(ret < 0)
				{
					printf("socket sigwait fail\n");
					break;
				}
				if(m.issetbit_err(_socket))
				{
					printf("socket has err bit\n");
					break;
				}
				if(!m.issetbit_in(_socket) &&
						!m.issetbit_out(_socket))
				{
					printf("socket no found in out signal\n");
					break;
				}

				int len = sizeof(int);
				int error = 0;
				ret = getsockopt(_socket, SOL_SOCKET, SO_ERROR, (void *)&error, (socklen_t *)&len);

				if(ret < 0 || (error != 0))
				{
					printf("socket can't getsocketopt error : %d  %d\n", errno, error);
					break;
				}
			}
			return true;
		}while(0);


		close_fds();
		return false;
	}
public:
	connection_manager(avfor_context  *avc,
			ui *interface) :
				manager(avc, interface),
				_th(nullptr),
				_socket(-1)
	{
			_pipe[0] = _pipe[1] = -1;
	}
	virtual ~connection_manager()
	{
		if(_pipe[1] != -1)
		{
			char a = 1;
			write(_pipe[1], &a, 1);
		}
		close_fds();
		if(_th)
		{
			_th->join();
			delete _th;
			_th = nullptr;
		}
	}
	void notify_to_main(unsigned code, void *ptr = nullptr)
	{
		struct pe_user u;
		u._code = code;
		u._ptr = ptr;
		_int->write_user(u);
	}


	virtual bool ready()
	{
		if(!connection_once())
		{
			printf("connection fail....\n");
			return false;
		}
		_th = new std::thread([&]()->void{
				DECLARE_LIVEMEDIA_NAMEDTHREAD("avfor connection manager");
				while(1)
				{
					fdmon mon;
					mon.set(_pipe[0], FD_IN | FD_ERR);
					mon.set(_socket, FD_IN | FD_ERR);
					mon.sigwait(-1);
					if(mon.issetbit_err(_socket) ||
							mon.issetbit_err(_pipe[0]) ||
							mon.issetbit_in(_pipe[0])/*allways in close*/)
					{
						notify_to_main(custom_code_section_connection_close);
						break;
					}
					if(mon.issetbit_in(_socket))
					{
						char buffer[512] = {0, };
						int res = recv(_socket, buffer, 500, MSG_DONTWAIT);
						if(res <= 0)
						{
							notify_to_main(custom_code_section_connection_close);
							break;
						}
						int len = strlen(buffer);
						char *p = (char *)malloc(len + 1);
						memcpy(p, buffer, len);
						p[len] = 0;

						send(_socket, "ACK-ok", 6, MSG_DONTWAIT);
						if(!strcmp("close", p))
						{
							notify_to_main(custom_code_section_connection_close);
							free(p);
							break;
						}
						notify_to_main(custom_code_section_connection_recv_command, (void *)p);
					}
				}
		});
		return _th != nullptr;
	}

	virtual void operator()(ui_event &e)
	{

	}
};


