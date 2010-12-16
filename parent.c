
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>

#include "tsserv.h"

int fork_parent(int pipefd[2], char* host, char* port, pid_t pid)
{
	int fd_s, status;

	close(pipefd[1]);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	openlog("tsserv", LOG_PID, LOG_USER);

	fd_s = start_listen(host, port);
	if(fd_s < 0)
	{
		closelog();
		return 1;
	}

	server_main(fd_s, pipefd[0]);

	close(pipefd[0]);

	shutdown(fd_s, SHUT_RDWR);
	close(fd_s);

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

	closelog();

	return 0;
}
