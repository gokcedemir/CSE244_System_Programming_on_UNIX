all: server clients

server: server.o 
	gcc -o server server.o -pthread -lm

clients: clients.o
	gcc -o clients clients.o -pthread -lm

server.o: server.c
	gcc -c server.c

clients.o: clients.c
	gcc -c clients.c

clean:
	rm *.o *.txt server clients; rm -rf logs

run1: 
	./server 8888 

run2: 
	./clients 3 4 3 8888

