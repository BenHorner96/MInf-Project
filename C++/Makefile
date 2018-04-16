CC=g++
CFLAGS= -Wall -pthread -std=c++11

all: main restart stop edit edit_safe set_camera

main:
	$(CC) $(CFLAGS) -o capture main.cpp config.cpp config.h

restart:
	$(CC) $(CFLAGS) -o restart restart.cpp config.cpp config.h

stop:
	$(CC) $(CFLAGS) -o stop stop.cpp config.cpp config.h

edit:
	$(CC) $(CFLAGS) -o edit_config edit_config.cpp config.cpp config.h

edit_safe:
	$(CC) $(CFLAGS) -o edit_config_safe edit_config_safe.cpp config.cpp config.h

set_camera:
	$(CC) $(CFLAGS) -o set_camera set_camera.cpp config.cpp config.h
