.PHONY: all clean depend install

TARGET = tsserv

CFLAGS += -g -Wall
LDFLAGS += -g -Wall

all: $(TARGET)

tsserv: child.o connect.o fds.o listen.o parent.o server.o transfer.o tsserv.o

clean:
	rm -f *.o

install: $(TARGET)
	cp $^ /usr/local/bin/

-include Makefile.dep

depend:
	$(CC) -MM *.c > Makefile.dep

