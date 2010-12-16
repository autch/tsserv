
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/select.h>
#include "tsserv.h"

int server_main(int fd_s, int fd_pipe)
{
	fd_set master;		// always maintain this
	int fd_peers[FD_SETSIZE];
	fd_set readfds;		// used for pselect(2)
	int nfds, ret;

	memset(fd_peers, -1, sizeof fd_peers);

	FD_ZERO(&master);
	FD_SET(fd_s, &master);
	FD_SET(fd_pipe, &master);
	nfds = (fd_pipe > fd_s) ? fd_pipe : fd_s;
	nfds++;

	for(;;)
	{
		readfds = master;
		ret = pselect(nfds, &readfds, NULL, NULL, NULL, NULL);
		if(ret < 0)
		{
			syslog(LOG_ERR, "pselect(2) failed: %s", strerror(errno));
			return 1;
		}
		if(ret > 0)
		{
			if(FD_ISSET(fd_s, &readfds))
			{
				if(connect_handler(fd_s, fd_peers) < 0)
					break;
			}
			if(FD_ISSET(fd_pipe, &readfds))
			{
				if(transfer_handler(fd_pipe, fd_peers) < 0)
					break;
			}
		}
	}

	return 0;
}
