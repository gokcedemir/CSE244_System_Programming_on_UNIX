all: grepSh

grepSh: grepSh.o
	gcc grepSh.o -o grepSh -pthread

grepSh.o: grepSh.c
	gcc -c grepSh.c

clean:
	rm *.o *.txt grepSh 
