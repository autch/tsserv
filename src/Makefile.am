AM_CFLAGS = -g -Wall @libevent_CFLAGS@


bin_PROGRAMS = tsserv
tsserv_SOURCES = child.c connect.c listen.c parent.c tsclient.c tsserv.c
tsserv_LDADD = @libevent_LIBS@ -lrt

