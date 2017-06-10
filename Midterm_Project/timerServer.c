/* 
 * File:   timerServer.c
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
#include <sys/time.h>
#include <sys/stat.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>

struct stat st = {0};

#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


#define ZERO 0
#define ONE 1

#define DIMENSION 20
#define BUFFERSIZE 25
#define SIZE_OF_ARR 255

/* Struct define  */
typedef struct MatrixForClients_t
{
	double structMatrix[DIMENSION][DIMENSION];
	int dimensionOfMatrix;
}MatrixForClients_t;

/*** Global variables ***/
//int clientPid[SIZE_OF_ARR];
int n=0; // argv[2]

void sigintHandler(int);

/*Determinant hesaplayan fonksiyon.*/
double determinant(double a[20][20],int n);

/*Alt matrislerin determinatını hesaplama*/
double determinantOfSubMatrix(int n,double arr[20][20], double *sub1, double *sub2,
					 double *sub3, double *sub4);
	
void catchingSignal(int sig, siginfo_t *siginfo, void *context);

									 
/*diger programlara kill gönderir*/
//void sendSignalOtherPrograms(int pid);	

			
/*
void initializeArr()
{
	int i=0;
	while(i< SIZE_OF_ARR)
	{
		clientPid[i]=-1;
		++i;
	}
}
*/
int main(int argc, char** argv)
{
	//unlink(argv[3]);
	char *fifoname;
	int fd,fd1, fd2;
	pid_t pid, pid1, pid2, serverPid;
	int i=0;
	FILE *fptr;
	int ytr=0;
	struct sigaction act;
	
	//usage kontrolu yapilir
	if(argc != 4)
	{
		fprintf(stderr,"Usage: %s <ticks_in_msec> <n> <mainpipename>\n",argv[0]);
		exit(ZERO);

	}

	signal(SIGINT, sigintHandler);
	
	fifoname = argv[3];
    n=atoi(argv[2]);

	if (stat("log", &st) == -1)
	{
   		 mkdir("log", 0700);
	}

	//serverin pidsi txt'ye yazilir.. 
	fptr= fopen("log/file.txt","w+");
	serverPid=getpid();
	fprintf(fptr,"%d",(int)serverPid);
	fclose(fptr);	

	act.sa_sigaction = catchingSignal;
	act.sa_flags= SA_SIGINFO;

	//seeWhattan gelen sinyali yakalama, bu kisim kitaptan alınmistir..
	if((sigemptyset(&act.sa_mask)== -1) || (sigaction(SIGUSR1, &act, NULL) ==-1))
	{
		perror("Failed to set SIGUSR1 to handle"); 
		return 1;
	}
	//initializeArr();
		
	/* mainpipename olusturma, server read yapcak */
	if(mkfifo(fifoname, FIFO_PERMS) == -1)
	{
		/* create a named pipe */
		 if (errno != EEXIST)
		 {
			fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",
							(long)getpid(),fifoname, strerror(errno));
			return 1;
		} 
	}

	//mainpipename clientların pidlerini okur
	while (((fd = open(fifoname, O_RDONLY)) == -1) && (errno == EINTR)) ;

	if (fd == -1 )
	{
		fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
		(long)getpid(), fifoname, strerror(errno));

		return 1;
	}

	//mainpipename clientların pidlerini okur..
	while(read(fd,&pid,sizeof(pid_t)) !=0);  //fifoyu okur
	 	


    while (1)
	{
        pause();   
    }        
 	
	
    return (EXIT_SUCCESS);
}

/* Sinyal yakalama fonksiyonu, yakalanan sinyalle matris olusturma 
Bu fonksiyon https://www.linuxprogrammingblog.com/code-examples/sigaction 
	alinmistir..                                             */
void catchingSignal(int sig, siginfo_t *siginfo, void *context)
{
	//printf("%d yakalandı.. sender id: %d \n",sig,(int)siginfo->si_pid);
	
	int controlPid=0;
	int fd1, fd2;
	
	struct timeval tv;
	unsigned long long millisecondsSinceEpoch;
	
	
/*bu siteden msecond cevirme alınmistir.. 
http://stackoverflow.com/questions/8558625/how-to-get-the-current-time-in-milliseconds-in-c-programming */
	gettimeofday(&tv, NULL);	 
   
	//her bir sinyal icin process olusturulur 
	controlPid= fork();

	if(controlPid == -1)
	{
		printf("Failed to fork..\n");
		exit(ZERO);
	}	
	else if(controlPid == ZERO ) // child process client ile ilgilenecek
	{		
		FILE *fptr;
		fptr= fopen("log/timerServer.log","a+"); 
		n=n*2;  //2nx2n'lik matris	
		struct MatrixForClients_t Smatrix;
		int yeah=0;
		double matrix[20][20];
		int x=0, y=0;		
		int i = 0;
		int j = 0;
		double detsub1=0.0,detsub2=0.0,detsub3=0.0,detsub4=0.0; //det of submatrix
		double detOfMatrix=0.0;
		char name1[BUFFERSIZE];  //fifo isimleri client icin 
		sprintf(name1,"%d.fifo",(int)siginfo->si_pid);
		do
		{
			sleep(1);
			srand(time(NULL));
			 /*Matris olusturma*/ 
			for(x = 0; x < n; x++)
			{
				for(y = 0; y < n; y++) 
			   		 matrix[x][y] = rand() % 100 + 1;
			}

			
			detOfMatrix= determinant(matrix,n);
			determinantOfSubMatrix(n,matrix,&detsub1,&detsub2,&detsub3,&detsub4);
			}while(detOfMatrix == 0.0 || detsub1 == 0.0 || detsub2 == 0.0 ||
								 detsub3 == 0.0 || detsub4 == 0.0);
		
		    millisecondsSinceEpoch=(unsigned long long)(tv.tv_sec)*1000+
								(unsigned long long)(tv.tv_usec) / 1000;
			fprintf (fptr,"Sender PID: %ld \n",(long)siginfo->si_pid);
			fprintf(fptr,"Time of %dx%d matrix generated : %llu \n", n,n,millisecondsSinceEpoch);
			fprintf(fptr,"Determinant Of Matrix: %.0f\n", detOfMatrix);
			fprintf(fptr,"\n\n");
			fclose(fptr);
			Smatrix.dimensionOfMatrix= n;
			
			for(i = 0; i < n; ++i)
			{
				for(j = 0; j < n; ++j)
				{
					Smatrix.structMatrix[i][j]= matrix[i][j];
				}
			}

		//matrisi göndermek icin fifo olusturulur
			if(mkfifo(name1, FIFO_PERMS) == -1)
			{
				/* create a named pipe */
				 if (errno != EEXIST)
				 {
					fprintf(stderr,"[%ld]:failed to create named pipe %s: %s\n",
									(long)getpid(),name1, strerror(errno));
					exit(ZERO);
				} 
			}
		while(((fd1 = open(name1, O_WRONLY)) == -1) && (errno == EINTR)) ;
		write(fd1,&Smatrix,sizeof(MatrixForClients_t));	
		close(fd1);
		
		exit(ZERO);
	}
	else // ana process childi bekler
    {
		wait(ZERO);
	}	
	
		
	/*
	if(unlink(name2) == -1)
	{
		perror("Server failed to unlink sequence FIFO");
	}
	*/

}		


/*diger programlara kill gönderir*/
/*void sendSignalOtherPrograms(int pid)
{
       kill(pid,SIGHUP);       
}
*/

void sigintHandler(int sig)
{
	FILE *fptr;
	fptr= fopen("log/timerServer.log","a+"); 
	/*int i=0;
	while(i<SIZE_OF_ARR)
	{
		if(clientPid[i] != -1 )
		{
			kill(clientPid[i],SIGKILL);
		}
		++i;
	}*/
	
    fprintf(fptr,"\nKilling process %d..\n",getpid());
    exit(0);
}

/* Alt matrislerin determinatını hesaplama */ 
double determinantOfSubMatrix(int n,double arr[20][20], double *dsub1, double *dsub2,
					 double *dsub3, double *dsub4)
{
	*dsub1 = 0.0, *dsub2 = 0.0, *dsub3 = 0.0, *dsub4 = 0.0;
	n = n/2; //nxn'lik matris
	double detsub1, detsub2, detsub3, detsub4;
	int i,j,k;
	int count = 0;
	double subMatrix1[20][20], subMatrix2[20][20], subMatrix3[20][20],
				subMatrix4[20][20];
    int x = 0, y = 0;
	while(count<4)
	{
	
	    
	    if(count == 0)
	    {
	    	x = 0; 
	    	for(i = 0; i < n; i++)
			{
				y = 0;
				for(j = 0; j < n; j++)
				{
					subMatrix1[i][j] = arr[x][y];
					y++;
				}
				x++;
			}
	    }
	    else if(count == 1)
	    {
	    	x = 0;
	    	for(i = 0; i < n; i++)
			{
				y = n;
				for(j = 0; j < n; j++)
				{
					subMatrix2[i][j] = arr[x][y];
					y++;
				}
				x++;
			}	
	    }
	    else if(count == 2)
	    {
	    	x = n;
	    	for(i = 0; i < n; i++)
			{
				y = 0;
				for(j = 0; j < n; j++)
				{
					subMatrix3[i][j] = arr[x][y];
					y++;
				}
				x++;
			}
	    }
	    else if(count == 3)
	    {
	    	x = n; 
	    	for(i = 0; i < n; i++)
			{
				y = n;
				for(j = 0; j < n; j++)
				{
					subMatrix4[i][j] = arr[x][y];
					y++;
				}
				x++;
			}
	    }
		
		if(count == 0)
		{			
			detsub1=determinant(subMatrix1, n);
			*dsub1 = detsub1;						
		}
		else if(count == 1)
		{				
			detsub2=determinant(subMatrix2, n);
			*dsub2 = detsub2;			
		}
		else if(count == 2)
		{		
			detsub3=determinant(subMatrix3, n);
			*dsub3 = detsub3;			
		}
		else if(count == 3)
		{			
			detsub4=determinant(subMatrix4, n);
			*dsub4 = detsub4;		
		}
		count++;		
	}
}


/******************************************************************************/
/*Bu kod http://paulbourke.net/miscellaneous/determinant/ sitesinden          */
/*		 alinmistir.                                                          */
/******************************************************************************/
double determinant(double a[20][20],int n)
{
   int i,j,j1,j2;
   double det = 0;
   double m[20][20] ;

   if (n < 1) { /* Error */

   } 
   else if (n == 1)
   { 
      det = a[0][0];
   } 
   else if (n == 2)
   {
      det = a[0][0] * a[1][1] - a[1][0] * a[0][1];
   } 
   else 
   {
      det = 0;
      for (j1=0;j1<n;j1++)
	  {
         for (i=1;i<n;i++)
		 {
            j2 = 0;
            for (j=0;j<n;j++)
		    {
               if (j == j1)
                  continue;
               m[i-1][j2] = a[i][j];
               j2++;
            }
         }
         det += pow(-1.0,1.0+j1+1.0) * a[0][j1] * determinant(m,n-1);
       
      }
   }
   return(det);
}


