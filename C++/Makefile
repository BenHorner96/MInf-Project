CC=g++
CFLAGS= -Wall -pthread -std=c++11

all: main restart start_stop

main:
	$(CC) $(CFLAGS) -o capture main.cpp config.cpp config.h

restart:
	$(CC) $(CFLAGS) -o restart restart.cpp config.cpp config.h

start_stop:
	$(CC) $(CFLAGS) -o start_stop start_stop.cpp config.cpp config.h
