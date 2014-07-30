
#ifndef tsserv_h
#define tsserv_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <evhttp.h>

#define DEFAULT_HOST NULL
#define DEFAULT_PORT "11234"
#define DEFAULT_HOST_HTTP "127.0.0.1"
#define DEFAULT_PORT_HTTP "8088"
#define DEFAULT_PATH_HTTP "/tsserv"
#define DEFAULT_LOGFILE "tsserv.stderr.log"

#define BEV_WM_LOWER (64 * 1024)
#define BEV_WM_UPPER (2048 * 1024)

#define BUFFER_SIZE (4096 * 1024)
#define LISTEN_BACKLOG 20

struct tsserv_config
{
    char* host;
    char* port;
    char* stderr_log;

    char* http_host;
    char* http_port;
	char* http_path;

	int bev_wm_lower;
	int bev_wm_upper;
};

struct tssparent_client
{
    struct bufferevent* bev;
	struct evhttp_request* req;
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

    struct evhttp* evhttp;

    struct bufferevent* pipe;

	uint8_t* buffer;
	size_t buffer_size;

    tsclient* clients;
};

int start_listen(struct tsserv_config* cfg, struct tssparent_context* ctx);
int server_main(int fd_s, int fd_pipe);

void connect_handler(struct evconnlistener* listener, evutil_socket_t sd,
		     struct sockaddr* peer, int socklen, void* user);

int fork_child(struct tsserv_config* cfg, int pipefd[2], char** av_cmd);
int fork_parent(struct tsserv_config* cfg, int pipefd[2], pid_t pid);

void http_connect_cb(struct evhttp_request* req, void* user);

#endif // !tsserv_h
