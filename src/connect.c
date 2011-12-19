
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "tsserv.h"

static void tsclientcb_event(struct bufferevent* bev, short what, void* user);
int tsclientSelectByBEV(struct tssparent_client* p, void* arg);

void connect_handler(struct evconnlistener* listener, evutil_socket_t sd,
					 struct sockaddr* peer, int socklen, void* user)
{
	struct tssparent_context* ctx = user;
	struct tssparent_client* client;
	struct bufferevent* bev;
	const int n_true = 1;

	bev = bufferevent_socket_new(ctx->ev_base, sd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if(!bev) return;

	client = tsclientNew();
	client->bev = bev;
	ctx->clients = tsclientPrepend(ctx->clients, client);

	evutil_make_socket_nonblocking(sd);
	evutil_make_socket_closeonexec(sd);
	setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &n_true, sizeof n_true);
	bufferevent_setcb(bev, NULL, NULL, tsclientcb_event, ctx);

	{
		struct sockaddr_storage* peer_addr = (struct sockaddr_storage*)peer;
        char peer_host[INET6_ADDRSTRLEN];
        uint16_t peer_port = 0;

		if(peer_addr->ss_family == AF_INET)
		{
            struct sockaddr_in* sa_in;
			sa_in = (struct sockaddr_in*)&peer;
			inet_ntop(peer_addr->ss_family, &sa_in->sin_addr, peer_host, sizeof peer_host);
			peer_port = ntohs(sa_in->sin_port);
            syslog(LOG_INFO, "Accept incoming connection from %s:%d", peer_host, peer_port);
		}
		else if(peer_addr->ss_family == AF_INET6)
		{	
            struct sockaddr_in6* sa_in6;
            sa_in6 = (struct sockaddr_in6*)&peer;
			inet_ntop(peer_addr->ss_family, &sa_in6->sin6_addr, peer_host, sizeof peer_host);
			peer_port = ntohs(sa_in6->sin6_port);
            syslog(LOG_INFO, "Accept incoming connection from [%s]:%d", peer_host, peer_port);
		}
	}
}

static void
tsclientcb_event(struct bufferevent* bev, short what, void* user)
{
	struct tssparent_context* ctx = user;
	struct tssparent_client* client;

	client = tsclientPick(ctx->clients, tsclientSelectByBEV, bev);
	if(client == NULL) return;

	if((what & BEV_EVENT_ERROR) || (what & BEV_EVENT_EOF))
	{
		ctx->clients = tsclientRemove(ctx->clients, client);
		tsclientFree(client);
	}
}

int tsclientSelectByBEV(struct tssparent_client* p, void* arg)
{
	struct bufferevent* bev = arg;
	return p->bev == bev;
}
