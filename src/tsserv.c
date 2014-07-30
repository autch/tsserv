// -*- mode: c; tab-width: 4; c-basic-offset: 4; -*-

#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tsserv.h"

static const struct option long_options[] =
{
	{ "debug",0, NULL, 'd' },
	{ "host", 1, NULL, 'h' },
	{ "port", 1, NULL, 'p' },
	{ "err",  1, NULL, 'e' },
	{ "help", 0, NULL, '?' },
	{ "http-host", 1, NULL, 'o' },
	{ "http-port", 1, NULL, 'r' },
	{ "http-path", 1, NULL, 't' }, 
	{ "bev-lower", 1, NULL, 'l' },
	{ "bev-upper", 1, NULL, 'u' },
	{ NULL,	  0, NULL,	0  },
};

int usage(int ac, char** av)
{
	printf("\nUsage: %s [-h HOST] [-p PORT] [-e LOGFILE] -- COMMAND...\n", *av);
	printf("  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 %s\n"
		   "  %-20s	 Lower watermark for transfer buffering (default: %d)\n"
		   "  %-20s	 Upper watermark for transfer buffering (default: %d)\n\n",
		   "-d, --debug",	  "Debug; Do not daemonize",
		   "-h, --host=HOST", "Local address to listen (default: 0.0.0.0)",
		   "-p, --port=PORT", "Local port to listen (default: " DEFAULT_PORT ")",
		   "-e, --err=LOGFILE", "Log stderr to LOGFILE (default: /dev/null)",
		   "-o, --http-host=HOST", "Local address to listen HTTP (default: " DEFAULT_HOST_HTTP ")",
		   "-r, --httt-port=PORT", "Local port to listen HTTP (default: " DEFAULT_PORT_HTTP ")",
		   "-t, --httt-path=PATH", "Local path to accept HTTP request (default: " DEFAULT_PATH_HTTP ")",
		   "-l, --bev-lower=BYTES", BEV_WM_LOWER,
		   "-u, --bev-upper=BYTES", BEV_WM_UPPER
		);
	
	return -1;
}

int main(int ac, char** av)
{
	pid_t pid;
	int pipefd[2]; // [ read, write ]
	int optchar, option_index = 0;
	char** av_cmd = NULL;
	struct tsserv_config config = {
		DEFAULT_HOST,
		DEFAULT_PORT,
		NULL,
		DEFAULT_HOST_HTTP,
		DEFAULT_PORT_HTTP,
		DEFAULT_PATH_HTTP,
		BEV_WM_LOWER,
		BEV_WM_UPPER
	};
	int debug = 0;

	for(;;) {
		optchar = getopt_long(ac, av, "dh:p:e:o:r:t:l:u:", long_options, &option_index);
		if(optchar == -1) break;

		switch(optchar) {
		case 'd': debug = 1; break;
		case 'h': config.host = optarg; break;
		case 'p': config.port = optarg; break;
		case 'e': config.stderr_log = optarg; break;
		case 'o': config.http_host = optarg; break;
		case 'r': config.http_port = optarg; break;
		case 't': config.http_path = optarg; break;
		case 'l': config.bev_wm_lower = atoi(optarg); break;
		case 'u': config.bev_wm_upper = atoi(optarg); break;
		case '?': return usage(ac, av);
		default:  printf("?? %c\n", optchar);
		}
	}

	if(optind >= ac) {
		fprintf(stderr, "%s: Missing commands\n", *av);
		return usage(ac, av);
	}
	
	av_cmd = av + optind;

	if(debug == 0) {
		pid = fork();
		if(pid == -1) {
			perror("fork (1st)");
			return 2;
		} else if(pid != 0)
			return 0;
		
		if(setsid() == (pid_t)-1) {
			perror("setsid");
			return 3;
		}
	}

	if(pipe(pipefd) == -1) {
		perror("pipe");
		return 4;
	}

	pid = fork();
	if(pid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		perror("fork (2nd)");
		return 5;
	}
	if(pid == 0)
		return fork_child(&config, pipefd, av_cmd);
	else
		return fork_parent(&config, pipefd, pid);
}

