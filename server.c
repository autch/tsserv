
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/select.h>
#include "tsserv.h"

int eval_nfds(int* fds);

int server_main(int fd_s, int fd_pipe)
{
	fd_set master_r, master_w;		// always maintain this
	int fd_peers[FD_SETSIZE];
	fd_set readfds, writefds;		// used for pselect(2)
	int nfds, ret, initial_nfds;

	memset(fd_peers, -1, sizeof fd_peers);

	FD_ZERO(&master_r);
	FD_SET(fd_s, &master_r);
	FD_SET(fd_pipe, &master_r);
	nfds = (fd_pipe > fd_s) ? fd_pipe : fd_s;
	nfds++;
    initial_nfds = nfds;

    FD_ZERO(&master_w);

	for(;;)
	{
		readfds = master_r;
        writefds = master_w;
		ret = pselect(nfds, &readfds, &writefds, NULL, NULL, NULL);
		if(ret < 0)
		{
			syslog(LOG_ERR, "pselect(2) failed: %s", strerror(errno));
			return 1;
		}
		if(ret > 0)
		{
			if(FD_ISSET(fd_s, &readfds))
			{
				if(connect_handler(fd_s, fd_peers, &master_w) < 0)
					break;
			}
			if(FD_ISSET(fd_pipe, &readfds))
			{
				if(transfer_handler(fd_pipe, fd_peers, &master_w, &writefds) < 0)
					break;
			}
		}
        nfds = eval_nfds(fd_peers);
        if(initial_nfds > nfds) nfds = initial_nfds;
	}

	return 0;
}

int eval_nfds(int* fds)
{
	int* p = fds;
	int* pe = fds + FD_SETSIZE;
    int max = -1;

	for(; p != pe; p++)
	{
        if(*p == -1) continue;

        if(*p > max) max = *p;
	}
	
	return max + 1;
}
