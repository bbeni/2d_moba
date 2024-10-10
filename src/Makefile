# Windows build

CC=gcc
CFLAGS=-Wall -I../raylib-5.0/include
LIBS=-l:libraylib.a -lm -lwinmm -lgdi32 -lws2_32
LDFLAGS=-L../raylib-5.0/lib
BUILDDIR=.build

SERVER_CFLAGS=-Wall
SERVER_LIBS=-lws2_32

.PHONY: all
all: main server

main: $(BUILDDIR)/main.o $(BUILDDIR)/common.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

server: $(BUILDDIR)/server.o $(BUILDDIR)/common.o
	$(CC) -o $@ $^ $(SERVER_CFLAGS) $(SERVER_LIBS)

$(BUILDDIR)/server.o: server.c $(BUILDDIR)
	$(CC) -c -o $@ $< $(SERVER_CFLAGS) 

$(BUILDDIR)/common.o: common.c $(BUILDDIR)
	$(CC) -c -o $@ $< $(SERVER_CFLAGS)

$(BUILDDIR)/main.o: main.c $(BUILDDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR):
	mkdir $(BUILDDIR)

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/*.o server.exe main.exe


