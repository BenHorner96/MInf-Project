CC=g++
CFLAGS= -Wall -pthread -std=c++11

all: main restart

main:
	$(CC) $(CFLAGS) -o test main.cpp config.cpp config.h

restart:
	$(CC) $(CFLAGS) -o restart restart.cpp config.cpp config.h

