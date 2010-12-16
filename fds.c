
#include <sys/select.h>
#include "tsserv.h"

int fds_add(int* fd_peers, int fd)
{
	int* p = fd_peers;
	int* pe = fd_peers + FD_SETSIZE;

	for(; p != pe; p++)
	{
		if(*p == fd)
			return p - fd_peers;

		if(*p == -1)
		{
			*p = fd;
			return p - fd_peers;
		}
	}
	
	return -1;
}

int fds_del(int* fd_peers, int fd)
{
	int* p = fd_peers;
	int* pe = fd_peers + FD_SETSIZE;

	for(; p != pe; p++)
	{
		if(*p == fd)
		{
			*p = -1;
			return p - fd_peers;
		}
	}
	
	return -1;
}
