CC = g++

CFLAGS = -Wall -O0

all: server client


server: server.o
	$(CC) $(CFLAGS) -DLDAP_DEPRECATED server.o -o vsys-server -lldap -llber -lm

server.o: src/server.cpp
	$(CC) $(CFLAGS) -DLDAP_DEPRECATED -c src/server.cpp -lldap -llber

client: client.o
	$(CC) $(CFLAGS) client.o -o vsys-client -lm

client.o: src/client.cpp
	$(CC) $(CFLAGS) -c src/client.cpp
        
clean:
	rm client.o server.o vsys-client vsys-server
