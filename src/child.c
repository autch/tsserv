
#include "tsserv.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

int fork_child(struct tsserv_config* cfg, int pipefd[2], char** av_cmd)
{
    int fd_null_w;
    char** avp = NULL;
    FILE* logfile = NULL;

    openlog("tsserv", LOG_PID, LOG_USER);

    close(pipefd[0]);
    if(dup2(pipefd[1], STDOUT_FILENO) != STDOUT_FILENO)
	syslog(LOG_ERR, "child: dup2(2) failed: %m");
    close(pipefd[1]);

    if(cfg->stderr_log)
    {
	fd_null_w = open(cfg->stderr_log, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if(fd_null_w >= 0)
	{
	    syslog(LOG_ERR, "child: Cannot open stderr log: %s: %m", cfg->stderr_log);
	    fd_null_w = open("/dev/null", O_WRONLY);
	}
	else
	{
	    logfile = fdopen(fd_null_w, "ab");
	    if(logfile)
	    {
		fprintf(logfile, "\n[%d -> %d] ", getppid(), getpid());
		for(avp = av_cmd; *avp; avp++)
		{
		    fprintf(logfile, "%s ", *avp);
		}
		fprintf(logfile, "\n");
		fflush(logfile);
	    }
	    else
		syslog(LOG_ERR, "child: Cannot fdopen for stderr log: %m");
	}
	// logfile is assoc'ed w/ fd_null_w, don't have to close both
    }
    else
	fd_null_w = open("/dev/null", O_WRONLY);

    dup2(fd_null_w, STDERR_FILENO);
    close(fd_null_w);
    close(STDIN_FILENO);
		
    execvp(*av_cmd, av_cmd);

    syslog(LOG_ERR, "child: Cannot exec %s: %m", *av_cmd);

    closelog();

    _exit(1);

    return 0;
}
