
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "tsserv.h"

int connect_handler(int epoll_fd, int fd_s)
{
	struct sockaddr_storage peer_addr;
	socklen_t addr_len;
	int new_socket;

	addr_len = sizeof peer_addr;
	new_socket = accept(fd_s, (struct sockaddr*)&peer_addr, &addr_len);
	if(new_socket < 0)
	{
		syslog(LOG_WARNING, "Cannot accept(2): %m");
		return -1;
	}
	else 
	{
        char peer_host[INET6_ADDRSTRLEN];
        uint16_t peer_port = 0;

		if(peer_addr.ss_family == AF_INET)
		{
            struct sockaddr_in* sa_in;
			sa_in = (struct sockaddr_in*)&peer_addr;
			inet_ntop(peer_addr.ss_family, &sa_in->sin_addr,
					  peer_host, sizeof peer_host);
			peer_port = ntohs(sa_in->sin_port);
            syslog(LOG_INFO, "Accept incoming connection from %s:%d",
                   peer_host, peer_port);
		}
		else if(peer_addr.ss_family == AF_INET6)
		{	
            struct sockaddr_in6* sa_in6;
            sa_in6 = (struct sockaddr_in6*)&peer_addr;
			inet_ntop(peer_addr.ss_family, &sa_in6->sin6_addr,
					  peer_host, sizeof peer_host);
			peer_port = ntohs(sa_in6->sin6_port);
            syslog(LOG_INFO, "Accept incoming connection from [%s]:%d",
                   peer_host, peer_port);
		}

        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = new_socket;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);
	}

	return 0;
}
