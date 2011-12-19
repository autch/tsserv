
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>

#include "tsserv.h"

int terminate = 0;

void sa_terminate(int sig);

static void pipecb_read(struct bufferevent* bev, void* user);
static void pipecb_event(struct bufferevent* bev, short what, void* user);
static void parent_free(struct tssparent_context* ctx);
static void evsignalcb_terminate(evutil_socket_t fd, short events, void* user);

int fork_parent(int pipefd[2], char* host, char* port, pid_t pid)
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

	fd_s = start_listen(ctx, host, port);
	if(fd_s < 0)
	{
		parent_free(ctx);
		closelog();
		return 1;
	}

	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

	ctx->pipe = bufferevent_socket_new(ctx->ev_base, pipefd[0], BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	bufferevent_setcb(ctx->pipe, pipecb_read, NULL, pipecb_event, ctx);
	bufferevent_setwatermark(ctx->pipe, EV_READ, BUFFER_SIZE, BUFFER_SIZE * LISTEN_BACKLOG);
	bufferevent_enable(ctx->pipe, EV_READ);

	event_base_dispatch(ctx->ev_base);

	if(ctx->clients != NULL)
	{
		tsclientFree(ctx->clients);
		ctx->clients = NULL;
	}

	bufferevent_free(ctx->pipe);
	ctx->pipe = NULL;

	shutdown(evconnlistener_get_fd(ctx->listener), SHUT_RDWR);
	evconnlistener_free(ctx->listener);
	ctx->listener = NULL;

	waitpid(pid, &status, 0);
	if(WIFEXITED(status))
	{
		syslog(LOG_INFO, "Process %d has exit w/ status %d", pid,
			   WEXITSTATUS(status));
	}
	else if(WIFSIGNALED(status))
	{
		syslog(LOG_INFO, "Process %d has terminated due to signal %d", pid,
			   WTERMSIG(status));
	}

	parent_free(ctx);
	closelog();

	return 0;
}

void sa_terminate(int sig)
{
    terminate = 1;
}

static void
parent_free(struct tssparent_context* ctx)
{
	if(ctx->clients != NULL)
		tsclientFree(ctx->clients);
	if(ctx->pipe != NULL)
		bufferevent_free(ctx->pipe);
	if(ctx->listener != NULL)
		evconnlistener_free(ctx->listener);
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

	if(fd == SIGCHLD)
	{
		event_base_loopbreak(ctx->ev_base);
		return;
	}
	kill(ctx->pid, fd);
}

static void
pipecb_read(struct bufferevent* bev, void* user)
{
	struct tssparent_context* ctx = user;
	char buffer[BUFFER_SIZE];
	size_t bytes_read;
	struct tssparent_client* client;
	int ret;

	bytes_read = bufferevent_read(ctx->pipe, buffer, sizeof buffer);

	STDLIST_FOREACH(ctx->clients, client)
	{
		// TODO: make this zerocopy
		ret = bufferevent_write(client->bev, buffer, bytes_read);
		if(ret != 0 && errno == EPIPE)
		{
			ctx->clients = tsclientRemove(ctx->clients, client);
			tsclientFree(client);
			return;
		}
	}
}

static void
pipecb_event(struct bufferevent* bev, short what, void* user)
{
	struct tssparent_context* ctx = user;

	if(what & BEV_EVENT_EOF)
	{
		event_base_loopbreak(ctx->ev_base);
	}

	if(what & BEV_EVENT_ERROR)
	{
		event_base_loopbreak(ctx->ev_base);
	}

}
