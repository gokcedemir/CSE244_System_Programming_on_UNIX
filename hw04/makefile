all: grepTh

grepTh: grepTh.o
	gcc grepTh.o -o grepTh -pthread

grepTh.o: grepTh.c
	gcc -c grepTh.c

clean:
	rm *.o *.txt grepTh 
