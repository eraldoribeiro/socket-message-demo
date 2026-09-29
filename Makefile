CC     = cc
CFLAGS = -Wall -Wextra -O2

all: server client

server: server.c msg.c msg.h
	$(CC) $(CFLAGS) -o $@ server.c msg.c

client: client.c msg.c msg.h
	$(CC) $(CFLAGS) -o $@ client.c msg.c

clean:
	rm -f server client
