
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include "tsserv.h"

static void pipecb_read(struct bufferevent* bev, void* user);
static void pipecb_event(struct bufferevent* bev, short what, void* user);
static void parent_free(struct tssparent_context* ctx);
static void evsignalcb_terminate(evutil_socket_t fd, short events, void* user);

int fork_parent(struct tsserv_config* cfg, int pipefd[2], pid_t pid)
{
    int fd_s, status;
    struct tssparent_context context;
    struct tssparent_context* ctx = &context;
    struct sigaction sa;

    close(pipefd[1]);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGPIPE, &sa, 0) != 0)
    {
		perror("failed to ignore SIGPIPE; sigaction");
		return -1;
    }

    openlog("tsserv", LOG_PID, LOG_USER);

    memset(ctx, 0, sizeof context);
    ctx->pid = pid;
    ctx->ev_base = event_base_new();

    ctx->evsignal_sigint = evsignal_new(ctx->ev_base, SIGINT, evsignalcb_terminate, ctx);
    ctx->evsignal_sigterm = evsignal_new(ctx->ev_base, SIGTERM, evsignalcb_terminate, ctx);
    ctx->evsignal_sigchld = evsignal_new(ctx->ev_base, SIGCHLD, evsignalcb_terminate, ctx);
    evsignal_add(ctx->evsignal_sigint, NULL);
    evsignal_add(ctx->evsignal_sigterm, NULL);
    evsignal_add(ctx->evsignal_sigchld, NULL);

    fd_s = start_listen(cfg, ctx);
    if(fd_s < 0) {
		parent_free(ctx);
		closelog();
		return 1;
    }

    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    ctx->pipe = bufferevent_socket_new(ctx->ev_base, pipefd[0], BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    bufferevent_setcb(ctx->pipe, pipecb_read, NULL, pipecb_event, ctx);
    bufferevent_setwatermark(ctx->pipe, EV_READ, cfg->bev_wm_lower, cfg->bev_wm_upper);
    bufferevent_enable(ctx->pipe, EV_READ);

	ctx->buffer_size = BUFFER_SIZE;
	ctx->buffer = malloc(ctx->buffer_size);

    event_base_dispatch(ctx->ev_base);

    if(ctx->clients != NULL) {
		tsclientFree(ctx->clients);
		ctx->clients = NULL;
    }

    bufferevent_free(ctx->pipe);
    ctx->pipe = NULL;

	free(ctx->buffer);
	ctx->buffer = NULL;

    shutdown(evconnlistener_get_fd(ctx->listener), SHUT_RDWR);
    evconnlistener_free(ctx->listener);
    ctx->listener = NULL;

    waitpid(pid, &status, 0);
    if(WIFEXITED(status)) {
		syslog(LOG_INFO, "Process %d has exit w/ status %d", pid, WEXITSTATUS(status));
    } else if(WIFSIGNALED(status)) {
		syslog(LOG_INFO, "Process %d has terminated due to signal %d", pid, WTERMSIG(status));
    }

    parent_free(ctx);
    closelog();

    return 0;
}

static void
parent_free(struct tssparent_context* ctx)
{
    if(ctx->clients != NULL)
		tsclientFree(ctx->clients);
    if(ctx->pipe != NULL)
		bufferevent_free(ctx->pipe);
	if(ctx->evhttp != NULL)
		evhttp_free(ctx->evhttp);
    if(ctx->listener != NULL)
		evconnlistener_free(ctx->listener);
	if(ctx->buffer != NULL)
		free(ctx->buffer);
    if(ctx->evsignal_sigint != NULL)
		event_free(ctx->evsignal_sigint);
    if(ctx->evsignal_sigterm != NULL)
		event_free(ctx->evsignal_sigterm);
    if(ctx->evsignal_sigchld != NULL)
		event_free(ctx->evsignal_sigchld);
    if(ctx->ev_base != NULL)
		event_base_free(ctx->ev_base);
}

static void
evsignalcb_terminate(evutil_socket_t fd, short events, void* user)
{
    struct tssparent_context* ctx = user;

    if(fd == SIGCHLD) {
		event_base_loopbreak(ctx->ev_base);
		return;
    }
    kill(ctx->pid, fd);
}

static void
pipecb_read(struct bufferevent* bev, void* user)
{
    struct tssparent_context* ctx = user;
    size_t bytes_read;
    tsclient* client;
    int ret;

    bytes_read = bufferevent_read(ctx->pipe, ctx->buffer, ctx->buffer_size);

#if 0
	{
		struct timeval tv;
		FILE* fp;

		gettimeofday(&tv, NULL);

		fp = fopen("stat.txt", "a+");
		if(fp) {
			fprintf(fp, "[%10ld.%06ld] %zu\n", tv.tv_sec, tv.tv_usec, bytes_read);
			fclose(fp);
		}
	}
#endif

    for(client = ctx->clients; client; client = client->next) {
		// TODO: make this zerocopy
		if(client->bev != NULL) {
			// this is a regular socket client
			ret = bufferevent_write(client->bev, ctx->buffer, bytes_read);
			if(ret != 0 && errno == EPIPE) {
				ctx->clients = tsclientDelete(ctx->clients, client);
				break;
			}
		}
		if(client->req != NULL) {
			// this is a http client
			struct evbuffer* evb = NULL;
			evb = evbuffer_new();
			evbuffer_add(evb, ctx->buffer, bytes_read);
			evhttp_send_reply_chunk(client->req, evb);
			if(evb != NULL) evbuffer_free(evb);
		}
    }
}

static void
pipecb_event(struct bufferevent* bev, short what, void* user)
{
    struct tssparent_context* ctx = user;

    if(what & BEV_EVENT_EOF) {
		event_base_loopbreak(ctx->ev_base);
    }

    if(what & BEV_EVENT_ERROR) {
		event_base_loopbreak(ctx->ev_base);
    }
}
