
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "tsserv.h"

int start_listen(char* host, char* port)
{
	struct addrinfo hints;
	struct addrinfo* result;
	struct addrinfo* rp;
	struct sockaddr_in* sa_in;
	struct sockaddr_in6* sa_in6;
	const int true = 1;
	int fd_s = -1, s = -1;
	char listen_host[INET6_ADDRSTRLEN];
	u_int16_t listen_port = 0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(host, port, &hints, &result);
	if(s)
	{
		syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(s));
		return -1;
	}

	for(rp = result; rp; rp = rp->ai_next)
	{
		fd_s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(fd_s == -1)
			continue;
		setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &true, sizeof true);
		if(bind(fd_s, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // okay

		close(fd_s);
	}

	if(!rp)
	{
		syslog(LOG_ERR, "Cannot find any bind(2)able host:port pair");
		close(fd_s);
		return -2;
	}

	if(listen(fd_s, LISTEN_BACKLOG) == -1)
	{
		syslog(LOG_ERR, "Cannot listen: %s", strerror(errno));
		close(fd_s);
		return -3;
	}

	if(rp->ai_family == AF_INET)
	{
		sa_in = (struct sockaddr_in*)rp->ai_addr;
		inet_ntop(rp->ai_family, &sa_in->sin_addr,
				  listen_host, sizeof listen_host);
		listen_port = ntohs(sa_in->sin_port);
        syslog(LOG_INFO, "Listening on %s:%d", listen_host, listen_port);
	}
	else if(rp->ai_family == AF_INET6)
	{
		sa_in6 = (struct sockaddr_in6*)rp->ai_addr;
		inet_ntop(rp->ai_family, &sa_in6->sin6_addr,
				  listen_host, sizeof listen_host);
		listen_port = ntohs(sa_in6->sin6_port);
        syslog(LOG_INFO, "Listening on [%s]:%d", listen_host, listen_port);
	}

	freeaddrinfo(result);

	return fd_s;
}
