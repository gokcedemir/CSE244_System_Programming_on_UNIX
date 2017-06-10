all: timerServer seeWhat showResults

timerServer: timerServer.o 
	gcc -o timerServer timerServer.o -lm

seeWhat: seeWhat.o
	gcc -o seeWhat seeWhat.o -lm

showResults: showResults.o
	gcc -o showResults showResults.o

timerServer.o: timerServer.c
	gcc -c timerServer.c

seeWhat.o: seeWhat.c
	gcc -c seeWhat.c

showResults.o: showResults.c
	gcc -c showResults.c

clean:
	rm *.o *.fifo timerServer seeWhat showResults; rm -rf log

run1: 
	./timerServer 1000 2 mpn 

run2: 
	./seeWhat mpn

run3: 
	./showResults 
