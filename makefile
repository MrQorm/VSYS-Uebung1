#makefile fuer Socketuebung
#Tobias Nemecek/Verena Poetzl

CC=gcc
CFLAGS=-g -Wall -O -pthread

all: fileserver client

fileserver: fileserver.c
	${CC} ${CFLAGS} fileserver.c -o fileserver

client: client.c
	${CC} ${CFLAGS} client.c -o client

clean:
	rm -f *.o fileserver client
