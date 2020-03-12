#include "WQ.h"

#define cond(f, s) {if(f){ printf("%s\n", s), exit(0);}}
#define MAX_STR_ARRAY_SIZE	512
#define SIGNAL_IN	(1 << 0)
#define SIGNAL_OUT	(1 << 1)
#define SIGNAL_ERR	(1 << 2)

struct _tsig
{
	struct timeval tv;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	int maxfd;
};
typedef struct _avfor_clients
{
	_avfor_clients *next;
	int socket;
	char ip[256];
	int (*in)(int sock, char *from);
	int (*out)(int sock, char *to, char *str);
	int (*error)(int sock, char *from);
}avfor_clients;

struct BackGround *signal_thread = NULL;
int pipeline[2];
struct _tsig *monitor = NULL;
int server_socket = -1;
int server_port = 12349;
avfor_clients *clients = NULL;
void memset_signal(struct _tsig * ptr)
{
	if(!ptr)
	{
		return;
	}
	ptr->tv.tv_sec = 0;
	ptr->tv.tv_usec = 0;
	FD_ZERO(&ptr->readfds);
	FD_ZERO(&ptr->writefds);
	FD_ZERO(&ptr->exceptfds);
	ptr->maxfd = -1;
}
struct _tsig *open_signal()
{
	struct _tsig *ptr = (struct _tsig *)libwq_malloc(sizeof( struct _tsig ));

	memset_signal(ptr);
	return ptr;
}
void close_signal(struct _tsig **pptr)
{
	if(pptr &&
			*pptr)
	{
		memset_signal((struct _tsig *)*pptr);
		libwq_free(*pptr);
		*pptr = NULL;
	}
}

void register_signal(struct _tsig * ptr, int fd, _dword flag)
{
	if(!ptr ||
			fd == -1)
	{
		return;
	}
	if(IS_BIT_SET(flag, SIGNAL_IN))
	{
		FD_SET(fd, &ptr->readfds);
	}
	if(IS_BIT_SET(flag, SIGNAL_OUT))
	{
		FD_SET(fd, &ptr->writefds);
	}
	if(IS_BIT_SET(flag, SIGNAL_ERR))
	{
		FD_SET(fd, &ptr->exceptfds);
	}
	if(ptr->maxfd <= fd)
	{
		ptr->maxfd = fd;
	}
}
int wait_signal(struct _tsig * ptr, int timeout)
{
	if(!ptr)
	{
		return WQEINVAL;
	}
	if(ptr->maxfd == -1)
	{
		return WQEBADF;
	}
	struct timeval *tm = NULL;
	if(timeout >= 0)
	{
		ptr->tv.tv_sec = timeout / 1000;
		ptr->tv.tv_usec = (timeout % 1000) * 1000;
		tm = &ptr->tv;
	}

	return select(ptr->maxfd + 1, &ptr->readfds, &ptr->writefds, &ptr->exceptfds, tm);
}
_dword bit_signal(struct _tsig * ptr, int fd)
{
	_dword bit = 0;
	if(!ptr)
	{
		return 0;
	}
	if(FD_ISSET(fd, &ptr->readfds))
	{
		BIT_SET(bit, SIGNAL_IN);
	}
	if(FD_ISSET(fd, &ptr->writefds))
	{
		BIT_SET(bit, SIGNAL_OUT);
	}
	if(FD_ISSET(fd, &ptr->exceptfds))
	{
		BIT_SET(bit, SIGNAL_ERR);
	}
	return bit;
}
static bool split_parse(char *org, char *str, char *str_ip)
{
	/*the format command >> 127.0.0.1*/
	int len = strlen(org);
	int nsplit = 0;
	int pos_split[2];
	pos_split[0] = -1;
	pos_split[1] = -1;
	for(int i = 0; i < MAX_STR_ARRAY_SIZE; i++)
	{
		if(org[i] == 0x3e)
		{
			if(pos_split[0] == -1) pos_split[0] = i;
			else if(pos_split[1] == -1) pos_split[1] = i;
			nsplit++;
		}
	}
	if(nsplit != 2)
	{
		return false;
	}
	if(pos_split[1] - pos_split[0] > 1)
	{
		return false;
	}
	if(pos_split[0]<=1)
	{
		return false;
	}
	if(org[pos_split[0] -1 ] != 0x20)
	{
		return false;
	}
	if(pos_split[1] + 4 >= len)
	{
		return false;
	}
	if(org[pos_split[1] + 1] != 0x20 )
	{
		return false;
	}
	memcpy(str, org, pos_split[0]-1);
	memcpy(str_ip, org + pos_split[1] + 2, len - pos_split[1] + 2);
	return true;
}

static int client_in(int sock, char *from)
{
	char buffer[MAX_STR_ARRAY_SIZE] = {0, };
	int res = recv(sock, buffer, MAX_STR_ARRAY_SIZE, MSG_DONTWAIT);
	if(res <= 0)
	{
		return -1;
	}
	printf("[in]%s : %s\n", from, buffer);
	return 0;

}
static int client_out(int sock, char *to, char *str)
{
	printf("[out]%s : %s\n", to, str);
	int len = strlen(str) + 1;
	int res = send(sock, str, len, MSG_DONTWAIT);
	return res == len ? 0 : -1;
}
static int client_error(int sock, char *from)
{
	printf("%s has been error\n", from);
	return -1;
}
static void client_close_all()
{
	avfor_clients *node = clients;
	while(node)
	{
		avfor_clients *next = node->next;
		close(node->socket);
		libwq_free(node);
		node = next;
	}
	clients = NULL;
}
static void handle_server_socket(_dword bit)
{
	if(bit & SIGNAL_IN)
	{
		avfor_clients *newclient = NULL;
		struct sockaddr_in server_client_addr;
		unsigned int size = 0;
		int sock = -1;
		int opt = 0;
		bzero(&server_client_addr, sizeof(struct sockaddr_in));
		size = sizeof(struct sockaddr_in);
		sock = accept(server_socket, (struct sockaddr *)&server_client_addr, &size);
		if(sock == -1)
		{
			printf("client socket accept fail\n");
			return;
		}
		opt = fcntl(server_socket, F_GETFL);
		if(opt < 0)
		{
			printf("client socket nonblock fail\n");
			close(sock);
			return;
		}
		opt |= (O_NONBLOCK | O_RDWR);
		if(fcntl(server_socket, F_SETFL, opt) < 0)
		{
			printf("client socket nonblock fail\n");
			close(sock);
			return;
		}
		newclient = (avfor_clients *)libwq_malloc(sizeof(avfor_clients));
		memset(newclient, 0, sizeof(avfor_clients));
		memcpy(newclient->ip, inet_ntoa(server_client_addr.sin_addr), strlen(inet_ntoa(server_client_addr.sin_addr)));
		newclient->socket = sock;
		newclient->in = client_in;
		newclient->out = client_out;
		newclient->error = client_error;
		printf("new client was incoming (%s)\n", newclient->ip);

		if(!clients)
		{
			clients = newclient;
			return;
		}
		avfor_clients *node = clients;
		while(node)
		{
			if(!node->next)
			{
				break;
			}
			node = node->next;
		}
		node->next = newclient;
	}
	if(bit & SIGNAL_ERR)
	{
		client_close_all();
		cond(true, "server socket broken");
	}
}
int handle_pipe(_dword bit)
{
	if(bit & SIGNAL_IN)
	{
		char buffer[MAX_STR_ARRAY_SIZE] = {0, };
		char buffer_str[MAX_STR_ARRAY_SIZE] = {0, };
		char buffer_ip[MAX_STR_ARRAY_SIZE] = {0, };
		int len = 0;
		int res = read(pipeline[0], buffer, MAX_STR_ARRAY_SIZE);
		if(res <= 0)
		{
			printf("pipe read less than zero continue....\n");
			return 0;
		}
		len = strlen(buffer);
		if(!strcmp("end server", buffer))
		{
			return -1;
		}
		if(split_parse(buffer, buffer_str, buffer_ip))
		{
			for(avfor_clients *node = clients; node; node = node->next)
			{
				if(!memcmp(buffer_ip, node->ip, strlen(node->ip)))
				{
					node->out(node->socket, node->ip, buffer_str);
				}
			}
		}
		else
		{
			printf("invalid format command\n");
		}
		return 0;
	}
	if(bit & SIGNAL_ERR)
	{
		client_close_all();
		cond(true, "pipe broken");
	}
	return 0;
}
void handle_clients()
{
	_dword bit = 0;
	avfor_clients *node = clients;
	avfor_clients *prev = NULL;
	avfor_clients *next = NULL;
	bool has_error = false;

	while(node)
	{
		next = node->next;
		bit = bit_signal(monitor, node->socket);
		if(bit & SIGNAL_IN)
		{
			has_error = node->in(node->socket, node->ip) < 0;
		}
		if((has_error) ||
				(bit & SIGNAL_ERR))
		{
			node->error(node->socket, node->ip);
			close(node->socket);
			libwq_free(node);

			if(!prev)
			{
				node = next;
				clients = node;
				continue;
			}
			node = next;
			prev->next = node;
			continue;
		}

		prev = node;
		node = next;
	}
}


static void signal_thread_function(void *p)
{

	struct sockaddr_in   server_addr;
	int opt = 0;
	int res = 0;

	server_socket =  socket(PF_INET, SOCK_STREAM, 0);
	cond(server_socket == -1, "server socket make fail");

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_port       = htons( server_port);
	server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
	cond(bind(server_socket, (struct sockaddr *)&server_addr,  sizeof(server_addr)) < 0, "server socket bind fail");
	cond(listen(server_socket, 5) < 0, "server socket listen fail");

	opt = fcntl(server_socket, F_GETFL);
	cond(opt < 0, "server socket nonblock fail")
	opt |= (O_NONBLOCK | O_RDWR);
	cond(fcntl(server_socket, F_SETFL, opt) < 0, "server socket nonblock fail");

	while(1)
	{
		memset_signal(monitor);

		register_signal(monitor, server_socket, SIGNAL_IN | SIGNAL_ERR);
		register_signal(monitor, pipeline[0], SIGNAL_IN | SIGNAL_ERR);
		for(avfor_clients *node = clients; node; node = node->next)
		{
			register_signal(monitor, node->socket, SIGNAL_IN | SIGNAL_ERR);
		}
		res = wait_signal(monitor, -1);
		cond(res <= 0, "signal waitting fail");/* no use timeout */

		{
			_dword bit = bit_signal(monitor, pipeline[0]);
			if(bit)
			{
				//printf("pipe notify\n");
			}
		}

		{
			_dword bit = bit_signal(monitor, server_socket);
			if(bit)
			{
				//printf("server notify\n");
			}
		}

		{
			for(avfor_clients *node = clients; node; node = node->next)
			{
				_dword bit = bit_signal(monitor, node->socket);
				if(bit)
				{
					//printf("client notify\n");
				}
			}
		}




		if(handle_pipe(bit_signal(monitor, pipeline[0])) < 0)
		{
			break;
		}
		handle_server_socket(bit_signal(monitor, server_socket));
		handle_clients();
	}
	client_close_all();
}

int main()
{
	printf("start avfor server\n");
	cond(pipe(pipeline) < 0, "pipe open fail");

	monitor = open_signal();
	cond(!monitor, "signal open fail");

	signal_thread = BackGround_turn(signal_thread_function, NULL);
	cond(!signal_thread, "thread open fail");


	printf("start input loop...\n");
	while(1)
	{
		char buffer[MAX_STR_ARRAY_SIZE] = {0, };
		gets(buffer);
		write(pipeline[1], buffer, MAX_STR_ARRAY_SIZE);
		if(!strcmp("end server", buffer))
		{
			break;
		}
	}
	printf("end input loop...\n");
	BackGround_turnoff(&signal_thread);
	close_signal(&monitor);
	close(pipeline[0]);
	close(pipeline[1]);
	printf("end avfor server\n");
	return 0;
}

