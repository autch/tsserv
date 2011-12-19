
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "tsserv.h"

int start_listen(struct tssparent_context* ctx, char* host, char* port)
{
	struct addrinfo hints;
	struct addrinfo* result;
	struct addrinfo* rp;
	struct sockaddr_in* sa_in;
	struct sockaddr_in6* sa_in6;
	int s = -1;
	char listen_host[INET6_ADDRSTRLEN];
	u_int16_t listen_port = 0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(host, port, &hints, &result);
	if(s)
	{
		syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(s));
		return -1;
	}

	for(rp = result; rp; rp = rp->ai_next)
	{
		ctx->listener = evconnlistener_new_bind(ctx->ev_base, connect_handler, ctx,
												LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
												-1, rp->ai_addr, rp->ai_addrlen);
		if(ctx->listener != NULL) break;
	}

	if(rp == NULL || ctx->listener == NULL)
	{
		syslog(LOG_ERR, "Cannot find any bind(2)able host:port pair");
		return -2;
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

	return 0;
}
