#makefile fuer Socketuebung
#Tobias Nemecek/Verena Poetzl

CC=gcc
CFLAGS=-g -Wall -O -pthread -std=gnu11

all: fileserver client

fileserver: fileserver.c
	${CC} ${CFLAGS} fileserver.c -o fileserver -lldap -llber -DLDAP_DEPRECATED

client: client.c
	${CC} ${CFLAGS} client.c -o client

clean:
	rm -f fileserver client
