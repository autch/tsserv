
#ifndef tsserv_h
#define tsserv_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#define DEFAULT_HOST NULL
#define DEFAULT_PORT "11234"
#define DEFAULT_LOGFILE "tsserv.stderr.log"
#define BUFFER_SIZE 4096
#define LISTEN_BACKLOG 20

struct tsserv_context
{
  char* host;
  char* port;
  char* stderr_log;
};

struct tssparent_client
{
	struct bufferevent* bev;
	struct tssparent_client* next;
};

typedef struct tssparent_client tsclient;

tsclient* tsclientNew();
void tsclientFree(tsclient* top);
tsclient* tsclientPrepend(tsclient* head, tsclient* node);
tsclient* tsclientFindByBEV(tsclient* head, struct bufferevent* bev);
tsclient* tsclientDelete(tsclient* top, tsclient* to_remove);

struct tssparent_context
{
	pid_t pid;

	struct event_base* ev_base;
	struct evconnlistener* listener;
	struct event* evsignal_sigint;
	struct event* evsignal_sigterm;
	struct event* evsignal_sigchld;

	struct bufferevent* pipe;

	tsclient* clients;
};

int start_listen(struct tssparent_context* ctx, char* host, char* port);
int server_main(int fd_s, int fd_pipe);

void connect_handler(struct evconnlistener* listener, evutil_socket_t sd,
					 struct sockaddr* peer, int socklen, void* user);

int fork_child(int pipefd[2], char** av_cmd, char* logfile);
int fork_parent(int pipefd[2], char* host, char* port, pid_t pid);

#endif // !tsserv_h
