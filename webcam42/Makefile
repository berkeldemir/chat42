CC = g++
CFLAGS = -std=c++11 $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

client: client.cpp
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f server client
