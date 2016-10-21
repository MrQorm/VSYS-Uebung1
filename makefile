#makefile fuer Socketuebung
#Tobias Nemecek/Verena Poetzl

CC=g++
CFLAGS=-g -Wall -O -std=c++11 -pthread

all: fileserver client

fileserver: fileserver.c
	${CC} ${CFLAGS} fileserver.c -o fileserver

client: client.c
	${CC} ${CFLAGS} client.c -o client

clean:
	rm -f *.o fileserver client
