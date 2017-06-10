/* 
 * File:   showResults.c
 * Author: Gökçe Demir - 141044067
 *
 * Created on April 11, 2017, 10:43 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include <errno.h>
#include <fcntl.h>



#define ZERO 0
#define ONE 1

/*Struct defines*/

//showResult icin
typedef struct MatrixForShowResult_t
{
	double shiftedResult;
	double convolution;
	int clientPid;
	float elapsed;
}MatrixForShowResult_t;

//ctrl+c handle etme
void sigintHandler(int);

int main(int argc, char** argv)
{
	int count=0;
    signal(SIGINT, sigintHandler);
	
	FILE *fptr;
	fptr=fopen("log/showResults.log","a+");

	while (1)
    {
		
		int fd2; //file descriptor
		char fifoname[50];
		struct MatrixForShowResult_t Rmatrix; //showresult icin

		if( argc != 1)
		{
			fprintf(stderr,"Usage: %s has no parameter.\n",argv[0]);
			exit(ZERO);
		}

		sprintf(fifoname,"fifoForShowResults");

		while((fd2 = open(fifoname, O_RDONLY))==-1);
		read(fd2,&Rmatrix,sizeof(MatrixForShowResult_t));
		printf("Sender Client_Pid: %d \n",Rmatrix.clientPid);
		printf("Result1: %f\n",Rmatrix.shiftedResult);
		fprintf(fptr,"m%d,  Sender Client_Pid: %d\n\n", count, Rmatrix.clientPid);
		fprintf(fptr,"Result1: %f  ",Rmatrix.shiftedResult);
		fprintf(fptr,"TimeElaps: %.20f\n\n",Rmatrix.elapsed);
	
		count++;
	   
		sleep(1);  
    }   
    fclose(fptr);
    return (EXIT_SUCCESS);
}

void sigintHandler(int sig)
{
   printf("\nKilling process %d..\n",getpid());
   exit(0);
}
