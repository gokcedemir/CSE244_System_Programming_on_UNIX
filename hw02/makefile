all: listdir

listdir: listdir.o
	gcc listdir.o -o listdir

listdir.o: listdir.c
	gcc -c listdir.c

clean:
	rm *.o listdir
