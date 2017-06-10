/* 
 * File:   seeWhat.c
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

#define BUFFERSIZE 25
#define DIMENSION 20

/* Function define */

/* matris işlemleri internetten yardım alınarak yapılmıştır. */
double determinant(double a[DIMENSION][DIMENSION],int n);
void transpose(double matrix[DIMENSION][DIMENSION],
				double matrix_cofactor[DIMENSION][DIMENSION],double size);
void cofactor(double matrix[DIMENSION][DIMENSION],double size);
void convolution(double in[DIMENSION][DIMENSION],
				double out[DIMENSION][DIMENSION], int size);
/********************************************************************/

/* Struct define  */
typedef struct MatrixForClients_t
{
	double structMatrix[DIMENSION][DIMENSION];
	int dimensionOfMatrix;
}MatrixForClients_t;


//showResult icin
typedef struct MatrixForShowResult_t
{
	double shiftedResult;
	double convolution;
	int clientPid;
   float elapsed;
}MatrixForShowResult_t;

/****************************************************************************/

/* Global variables */
pid_t serverPid;
int counter =0;
double shiftedInverseMatrix[DIMENSION][DIMENSION];
double convolutionMatrix[DIMENSION][DIMENSION];

void sigintHandler(int sig);
//int sendSignalOtherPrograms(int pid);


float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

int main(int argc, char** argv)
{
	int fd, fd1, fd2;
	char *fifoname;
    pid_t pid, pid1, pid2;	
	FILE *fptr;
	int serverPid1; 
	int cPid;
	int i=0,j=0;
	int shiftedPid;
	int convolutionPid;
	struct MatrixForClients_t Smatrix;   //clientlar icin
	struct MatrixForShowResult_t Rmatrix; //showresult icin
	int n=0; //dimeonsion of matrix
	double matrix[DIMENSION][DIMENSION];
	FILE *fileptr;
	char filename[BUFFERSIZE]; 
	char name1[BUFFERSIZE];  //fifo ismi client icin 
	char fifo[BUFFERSIZE];
	int check=0;
     struct timeval t0;
	   struct timeval t1;
	   float elapsed;

	//usage kontrolu
	if(argc != 2)
	{
		fprintf(stderr,"Usage: %s <mpipename> \n",argv[0]);
		exit(ZERO);	
	}	

    signal(SIGINT, sigintHandler);	

	if (stat("log", &st) == -1)
	{
   		 mkdir("log", 0700);
	}

	fifoname=argv[1];
	fptr= fopen("log/file.txt","r");
	fscanf(fptr,"%d",&serverPid1);
	fclose(fptr);
	serverPid=(pid_t)serverPid1;

	sprintf(fifo,"fifoForShowResults");
	
	while(1)
	{
		kill(serverPid,SIGUSR1); //servera sinyal gönderilir	
	
		
	
		/*timerServerdan gelen matrisi alır*/
		sprintf(name1,"%d.fifo",(int)getpid());
		while((fd1 = open(name1, O_RDONLY))==-1);
		read(fd1,&Smatrix,sizeof(MatrixForClients_t));

		
		shiftedPid=fork(); //shifted inverse icin


		//shifted islemi icin process olusturulur..
		if(shiftedPid==-1)
		{
			printf("Failed to fork..\n");
			exit(ZERO);
		}
		else if(shiftedPid == 0) //child process shifted fonkunu cagır
		{
			int count=0;
			double subMatrix1[DIMENSION][DIMENSION], 
					subMatrix2[DIMENSION][DIMENSION], 
					subMatrix3[DIMENSION][DIMENSION],
				    subMatrix4[DIMENSION][DIMENSION];
			
			double detOriginalMatrix , detShiftedMatrix;
			double result1=0.0; //shifted matris yapar bunu
			double sonuc1, sonuc2;
			int x = 0, y = 0;
			int i,j,k;
			
			cPid=(int)getpid();
			Rmatrix.clientPid= cPid;
			//seeWhat icin log dosyası olusturma
			sprintf(filename,"log/seeWhat_%d.log",(int)getpid());
			fileptr=fopen(filename,"a+");	


			fprintf(fileptr,"Original Matrix: [ ");
			for(i = 0; i < Smatrix.dimensionOfMatrix; ++i)
			{
				for(j = 0; j < Smatrix.dimensionOfMatrix; ++j)
				{
						
						matrix[i][j]=Smatrix.structMatrix[i][j];
						fprintf(fileptr,"%.0f ",matrix[i][j]);
				}
				if(i != Smatrix.dimensionOfMatrix-1)
					fprintf(fileptr,"; ");
			}	
			fprintf(fileptr,"]\n\n");
			n=Smatrix.dimensionOfMatrix;		
			n=n/2;   
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
							subMatrix1[i][j] =matrix[x][y];
							y++;
						}
						x++;
					}
					counter=0;
					cofactor(subMatrix1,n);	
				}
				else if(count == 1)
				{
					x = 0;
					for(i = 0; i < n; i++)
					{
						y = n;
						for(j = 0; j < n; j++)
						{
							subMatrix2[i][j] =matrix[x][y];
							y++;
						}
						x++;
					}
					counter=1;
					cofactor(subMatrix2,n);	
				}
				else if(count == 2)
				{
					x = n;
					for(i = 0; i < n; i++)
					{
						y = 0;
						for(j = 0; j < n; j++)
						{
							subMatrix3[i][j] = matrix[x][y];
							y++;
						}
						x++;
					}
					counter=2;
					cofactor(subMatrix3,n);
				}
				else if(count == 3)
				{
					x = n; 
					for(i = 0; i < n; i++)
					{
						y = n;
						for(j = 0; j < n; j++)
						{
							subMatrix4[i][j] = matrix[x][y];
							y++;
						}
						x++;
					}
					counter=3;
					cofactor(subMatrix4,n);
				}
			count++;	
			}
			n=n*2;
			fprintf(fileptr,"Shifted Inverse Matrix: [");
			for (i=0;i<n;i++)
			{								
				for (j=0;j<n;j++)
				{
					fprintf(fileptr,"%f ",shiftedInverseMatrix[i][j]);
				}
				if(i != (n-1))
					fprintf(fileptr,"; ");
			}	
			fprintf(fileptr,"]\n");

 		    gettimeofday(&t0, 0);
			sonuc1=	 determinant(matrix,n);
			sonuc2= determinant(shiftedInverseMatrix,n);
			result1= sonuc1- sonuc2;

			Rmatrix.shiftedResult = result1;
  			 gettimeofday(&t1, 0);
			 elapsed = timedifference_msec(t0, t1);
			Rmatrix.elapsed=elapsed;


			//showResulta result1, result2 ve timeElaps'ı gondermek icin fifo olusturma
			if(mkfifo(fifo, FIFO_PERMS) == -1)
			{
				/* create a named pipe */
				 if (errno != EEXIST)
				 {
					fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",
							(long)getpid(),fifo, strerror(errno));
					return 1;
				} 
			}
		
			//fifoForShowResults 
			while ((fd2 = open(fifo, O_WRONLY)) == -1 ) ;

			if (fd2 == -1 )
			{
				fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
				(long)getpid(), fifo, strerror(errno));
				return 1;
			}
			check = write(fd2,&Rmatrix,sizeof(MatrixForShowResult_t));	
			

			fclose(fileptr);
			exit(ZERO);
		}
		else // ana process childi bekler
		{
			wait(ZERO);
		}

	}

		// write icin main fifo acilir
		while (((fd = open(fifoname, O_WRONLY)) == -1) && (errno == EINTR)) ;
		if(fd==-1 )
		{
			perror("Client failed to open FIFO");
			return 1;	
		}
		 /* main Fifoya pid yazilir*/
		pid = getpid();
		write(fd,&pid,sizeof(pid_t)); 

	
    return (EXIT_SUCCESS);
}

/*diger programlara kill gönderir*/
int sendSignalOtherPrograms(int pid)
{
       kill(pid,SIGKILL);       
}


void sigintHandler(int pid)
{
	//servera ctlr+c geldiginde kill gönderir
	kill(serverPid,SIGKILL);
    printf("\nKilling process %d..\n",getpid());
    exit(0);
}

 /*        Kofaktor matrisi hesaplama                              */
/*Dipnot:https://www.codeproject.com/Questions/754429/C-Program-to-calculate-inverse-of-matrix-n-n
bu siteden alinmistir..*/
void cofactor(double matrix[20][20],double size)
{
     double m_cofactor[20][20],matrix_cofactor[20][20];
     int p,q,m,n,i,j;
     for (q=0;q<size;q++)
     {
         for (p=0;p<size;p++)
         {
             m=0;
             n=0;
             for (i=0;i<size;i++)
             {
                 for (j=0;j<size;j++)
                 {
                     if (i != q && j != p)
                     {
                        m_cofactor[m][n]=matrix[i][j];
                        if (n<(size-2))
                           n++;
                        else
                        {
                            n=0;
                            m++;
                        }
                     }
                 }
             }
             matrix_cofactor[q][p]=pow(-1,q + p) * determinant(m_cofactor,size-1);
         }
     }
     transpose(matrix,matrix_cofactor,size);
}


/*Finding transpose of cofactor of matrix*/ 
void transpose(double matrix[DIMENSION][DIMENSION],
				double matrix_cofactor[DIMENSION][DIMENSION],double size)
{
	
	int i,j;
	float m_transpose[DIMENSION][DIMENSION],m_inverse[DIMENSION][DIMENSION],d;
	int a=0;
	int b=0;

	for (i=0;i<size;i++)
	{
		for (j=0;j<size;j++)
		{
	 		m_transpose[i][j]=matrix_cofactor[j][i];
		}
	}
	d=determinant(matrix,size);
	for (i=0;i<size;i++)
	{
		for (j=0;j<size;j++)
		{
			 m_inverse[i][j]=m_transpose[i][j]/d;
		}
	}
	if(counter == 0)
	{
		a=0;
		for (i=0;i<size;i++)
		{	
			b=0;				
			for (j=0;j<size;j++)
			{
				shiftedInverseMatrix[a][b]=m_inverse[i][j];
				b++;
			}
			a++;
		}
    }
	else if(counter==1)
	{
		a=0;
		for (i=0;i<size;i++)
		{	
			b=size;				
			for (j=0;j<size;j++)
			{
				shiftedInverseMatrix[a][b]=m_inverse[i][j];
				b++;
			}
			a++;
		}		
	}
	else if(counter==2)
	{
		a=size;
		for (i=0;i<size;i++)
		{	
			b=0;				
			for (j=0;j<size;j++)
			{
				shiftedInverseMatrix[a][b]=m_inverse[i][j];
				b++;
			}
			a++;
		}		
	}
	else if(counter==3)
	{
		a=size;
		for (i=0;i<size;i++)
		{	
			b=size;				
			for (j=0;j<size;j++)
			{
				shiftedInverseMatrix[a][b]=m_inverse[i][j];
				b++;
			}
			a++;
		}		
	}
    
}

/******************************************************************************/
/*Bu kod http://paulbourke.net/miscellaneous/determinant/ sitesinden          */
/*		 alinmistir.                                                          */
/******************************************************************************/
double determinant(double a[DIMENSION][DIMENSION],int n)
{
   int i,j,j1,j2;
   double det = 0;
   double m[DIMENSION][DIMENSION] ;

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


