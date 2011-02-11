
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/socket.h>

#include "tsserv.h"

/**
 * @return 0 when success, 1 when some socket has been closed, 2 when
 * some error(s) occured, -1 when stdin has some error
 */
int transfer_handler(int fd_pipe, int* fd_peers, fd_set* master_w, fd_set* writefds)
{
	int* pfd;
	int* pfde;
	uint8_t buffer[BUFFER_SIZE];
	ssize_t bytes_read, bytes_sent;
	int fd, ret = 0;

	bytes_read = read(fd_pipe, buffer, sizeof buffer);
	if(bytes_read < 0)
	{
		syslog(LOG_ERR, "Cannot read(2) from stdin: %s", strerror(errno));
		return -1;
	}
	if(bytes_read == 0)
	{
		syslog(LOG_INFO, "EOF from stdin");
		return -1;
	}
		
	for(pfd = fd_peers, pfde = pfd + FD_SETSIZE; pfd != pfde; pfd++)
	{
		fd = *pfd;
		if(fd == -1) continue;
        if(!FD_SET(fd, writefds)) continue;
		
		bytes_sent = send(fd, buffer, bytes_read, MSG_NOSIGNAL | MSG_DONTWAIT);
		if(bytes_sent < 0)
		{
			switch(errno)
			{
			case EAGAIN:
//			case EWOULDBLOCK:
				break;
			case EPIPE:
			case ECONNRESET:
				syslog(LOG_INFO, "Connection closed by peer %d", fd);
				fds_del(fd_peers, fd);
				shutdown(fd, SHUT_RDWR);
				close(fd);
                FD_CLR(fd, master_w);
				ret = 1;
				break;
			default:
				syslog(LOG_WARNING, "Cannot send(2) to %d - closing: %s",
					   fd, strerror(errno));
				fds_del(fd_peers, fd);
				shutdown(fd, SHUT_RDWR);
				close(fd);
                FD_CLR(fd, master_w);
				ret = 2;
			}
		}
	}

	return ret;
}
