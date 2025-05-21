CC = g++
CFLAGS = -std=c++11 -Wall -Wextra $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

all: webcam_p2p

webcam_p2p: webcam_p2p.cpp
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f webcam_p2p

run:
	./webcam_p2p
