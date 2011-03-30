
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "tsserv.h"

extern int terminate;

int server_main(int fd_s, int fd_pipe)
{
	int nfds, ret;
	int epoll_fd;
	size_t max_events = 16;
	int conns = 0;
	struct epoll_event ev;
	struct epoll_event* events = NULL;

	epoll_fd = epoll_create(10);
	if(epoll_fd < -1)
	{
		syslog(LOG_ERR, "epoll_create(2) failed: %m");
		return 1;
	}

	events = calloc(max_events, sizeof ev);

	ev.events = EPOLLIN;
	ev.data.fd = fd_s;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_s, &ev);
	ev.data.fd = fd_pipe;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_pipe, &ev);

	while(!terminate)
	{
		nfds = epoll_pwait(epoll_fd, events, max_events, 20, NULL);
		if(nfds < 0)
		{
			syslog(LOG_ERR, "epoll_wait failed: %m");
			break;
		}

		int i;
		for(i = 0; i < nfds; i++)
		{
			if(events[i].data.fd == fd_s)
			{
				if(connect_handler(epoll_fd, fd_s) < 0)
					break;
				conns++;
				if(conns == max_events)
				{
					max_events += 4;
					events = realloc(events, max_events * sizeof ev);
				}
			}
			if(events[i].data.fd == fd_pipe)
			{
				uint8_t buffer[BUFFER_SIZE];
				ssize_t bytes_read, bytes_sent;

				bytes_read = read(fd_pipe, buffer, sizeof buffer);
				if(bytes_read < 0)
				{
					syslog(LOG_ERR, "Cannot read(2) from stdin: %m");
                    terminate = 1;
					break;
				}
				if(bytes_read == 0)
				{
					syslog(LOG_INFO, "EOF from stdin");
                    terminate = 1;
					break;
				}
				
				int j;
				for(j = 0; j < nfds; j++)
				{
					int fd = events[j].data.fd;
					if(fd == fd_s) continue;
					if(fd == fd_pipe) continue;

					bytes_sent = send(fd, buffer, bytes_read, MSG_NOSIGNAL | MSG_DONTWAIT);
					if(bytes_sent < 0)
					{
						switch(errno)
						{
						case EAGAIN:
							break;
						case EPIPE:
						case ECONNRESET:
							syslog(LOG_INFO, "Connection closed by peer %d", fd);
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
							shutdown(fd, SHUT_RDWR);
							close(fd);
							conns--;
							ret = 1;
							break;
						default:
							syslog(LOG_WARNING, "Cannot send(2) to %d - closing: %m", fd);
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
							shutdown(fd, SHUT_RDWR);
							close(fd);
							conns--;
							ret = 2;
						}
						
					}
				}

			}
		}
	}

	close(epoll_fd);

	return 0;
}
