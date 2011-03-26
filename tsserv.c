// -*- mode: c; tab-width: 4; c-basic-offset: 4; -*-

#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "tsserv.h"

static const struct option long_options[] =
{
    { "debug",0, NULL, 'd' },
	{ "host", 1, NULL, 'h' },
	{ "port", 1, NULL, 'p' },
	{ "err",  1, NULL, 'e' },
	{ "help", 0, NULL, '?' },
	{ NULL,	  0, NULL,	0  },
};

int usage(int ac, char** av)
{
	printf("\nUsage: %s [-h HOST] [-p PORT] [-e LOGFILE] -- COMMAND...\n", *av);
	printf("  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n\n",
           "-d, --debug",     "Debug; Do not daemonize",
		   "-h, --host=HOST", "Local address to listen (default: 0.0.0.0)",
		   "-p, --port=PORT", "Local port to listen (default: " DEFAULT_PORT ")",
		   "-e, --err=LOGFILE", "Log stderr to LOGFILE (default: /dev/null)");
	return -1;
}


int main(int ac, char** av)
{
	pid_t pid;
	int pipefd[2]; // [ read, write ]
	int optchar, option_index = 0;
	char** av_cmd = NULL;
	struct tsserv_context context = { DEFAULT_HOST, DEFAULT_PORT, NULL };
    int debug = 0;

	for(;;)
	{
		optchar = getopt_long(ac, av, "dh:p:e:", long_options, &option_index);
		if(optchar == -1) break;

		switch(optchar)
		{
		case 'd': debug = 1; break;
		case 'h': context.host = optarg; break;
		case 'p': context.port = optarg; break;
		case 'e': context.stderr_log = optarg; break;
		case '?': return usage(ac, av);
		default:  printf("?? %c\n", optchar);
		}
	}

	if(optind >= ac)
	{
		fprintf(stderr, "%s: Missing commands\n", *av);
		return usage(ac, av);
	}
	
	av_cmd = av + optind;

    if(debug == 0)
    {
        pid = fork();
        if(pid == -1)
        {
            perror("fork (1st)");
            return 2;
        }
        else if(pid != 0)
            return 0;
        
        if(setsid() == (pid_t)-1)
        {
            perror("setsid");
            return 3;
        }
    }

	if(pipe(pipefd) == -1)
	{
		perror("pipe");
		return 4;
	}

	pid = fork();
	if(pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		perror("fork (2nd)");
		return 5;
	}
	if(pid == 0)
		return fork_child(pipefd, av_cmd, context.stderr_log);
	else
		return fork_parent(pipefd, context.host, context.port, pid);
}

