
#ifndef tsserv_h
#define tsserv_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#define DEFAULT_HOST NULL
#define DEFAULT_PORT "11234"
#define DEFAULT_LOGFILE "/tmp/tsserv.stderr.log"
#define BUFFER_SIZE 4096
#define LISTEN_BACKLOG 20

struct tsserv_context
{
  char* host;
  char* port;
  char* stderr_log;
};

int start_listen(char* host, char* port);
int server_main(int fd_s, int fd_pipe);

int connect_handler(int epoll_fd, int fd_s);

int fork_child(int pipefd[2], char** av_cmd, char* logfile);
int fork_parent(int pipefd[2], char* host, char* port, pid_t pid);

#endif // !tsserv_h
