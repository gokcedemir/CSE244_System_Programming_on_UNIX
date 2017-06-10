/* 
 * File:   server.c
 * Author: Gökçe Demir - 141044067
 * System_Programming Final Project
 * Created on May 22, 2017, 10:43 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h> // for socket
#include <netinet/in.h> // for struct sockadrr_in
#include <netdb.h>      // for socket
#include <pthread.h>
#include <arpa/inet.h> //inet_addr
#include <resolv.h>
#include <semaphore.h>
#include <sys/ipc.h> 
#include <sys/shm.h>   //shared mem
#include <sys/syscall.h> // thread id

#define ZERO 0
#define ONE 1
#define SIZE_ARR 255
#define MAX_SIZE 1024
#define MAXHOSTNAME 255
#define SHM_SIZE 10024
#define DIMENSION 20

#define PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct Parameter_t{
   int row;		//matrix size icin thread fonksiyonunu gonderme
   int col;
   unsigned short portnumber;
  
}Parameter_t;

typedef struct Sender_t
{
	double matrisA[20][20];
	double matrisB[20][1];
	double X[20][1];
	double errorNorm;
}Sender_t;

typedef struct  Forverify_t
{
	double X[DIMENSION][ONE];
	double inverse[20][20];
}Forverify_t;

typedef struct P2toP3_t
{
	double matrisA[20][20];
	double matrisB[20][1];
	double X[20][1];
}P2toP3_t;

/*** Global Variables *****/
sem_t semlockp;
char *sharedtotal; // for shared memory buffer
Sender_t yeter;


//bu fonksiyonlar kitaptan alınmıştır. Chapter 13.
int writeForData(int s, void *buf, int n);
int readForData(int s, void *buf, int n);

// matris generate eden fonksiyon  --> P1
void generate(int col, int row,int socketfd);

// A.x= B lineer denklemini çözen fonksiyon  --> P2
void solve(int row,int col);

// error hesaplama -- > P3
void verify(int row, int col, int sockfd);

/******************************************************************************/
/*Bu kod http://paulbourke.net/miscellaneous/determinant/ sitesinden          */
/*		 alinmistir.                                                          */
double determinant(double a[20][20],int n);

/*Finding transpose of cofactor of matrix*/ 
void transpose(double matrix[DIMENSION][DIMENSION],
				double matrix_cofactor[DIMENSION][DIMENSION],double size);

 /*        Kofaktor matrisi hesaplama                              */
/*Dipnot:https://www.codeproject.com/Questions/754429/C-Program-to-calculate-inverse-of-matrix-n-n
bu siteden alinmistir..*/
void cofactor(double matrix[20][20],double size);
/******************************************************************************/

//accept yapan fonksiyon
int getConnect(int socketfd);

//sinyal yakalama fonksiyonu
void sigintHandler(int sig);

//thread fonksiyonu
void* connection(void *arg);


int main(int argc, char** argv)
{
	unlink("id.txt");
	FILE *f;
	
	//usage kontrolu yapilir
	if(argc !=2)
	{
		fprintf(stderr,"Usage: %s <port #, id> <thpool size, k>\n",argv[0]);
		exit(ZERO);
	}
	f=fopen("control.txt","a+");
	fclose(f);
    signal(SIGINT, sigintHandler);	
    int portnum= atoi(argv[1]);  // parameter of "id"
    int numOfConnectedClient; // number of clients currently


	 //thread-per-request implementations
	
	char *message;
	char clientMessage[MAX_SIZE];
 	char myname[MAXHOSTNAME+ONE];
    int socketfd;
	struct sockaddr_in sa;
	struct hostent *hp;
	pthread_t tid;
	int ret;

    
    if(sem_init(&semlockp, ZERO, ONE) == -1)
	{
		perror("Failed to initialize semaphore");
		return ONE;
	}

	//kitaptan yararlanılarak yapılmıştır -- > chapter 13...
	memset(&sa, 0, sizeof(struct sockaddr_in));
	gethostname(myname, MAXHOSTNAME);
	
	if ((hp=gethostbyname(myname)) == NULL)
		return -1;
	

  	sa.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhosta baglanırız
	sa.sin_family= hp->h_addrtype;
	sa.sin_port= htons(portnum);
	
	//socket olusturma
	if ((socketfd= socket(AF_INET, SOCK_STREAM, ZERO )) < ZERO)
		return -1;

	if (bind(socketfd,(struct sockaddr *)&sa,sizeof(struct sockaddr_in)) < 0)
 	{
		close(socketfd);
		return -1;
	}
	listen(socketfd, 3);

	while(ONE)
	{
		if ((ret= getConnect(socketfd)) < ZERO)
	    {
			if (errno == EINTR)
				continue;
			perror("accept");
			exit(ONE);
		}
		++numOfConnectedClient;
		printf("# of connection of currently client : %d\n",numOfConnectedClient );
		if(pthread_create(&tid, NULL, connection,(void*)&ret) < ZERO)
		{
			perror("connection error");
			return ONE;
		}
	}


	return ZERO;
}

/*bu fonksiyon kitaptan alınmıştır */
int getConnect(int socketfd)
{
	int ret;
	if((ret = accept(socketfd,NULL,NULL)) < ZERO)
		return -1;
	return ret;
}

void* connection(void *arg)
{
	int newsock = *(int *)arg;
	char *sendMessage="hello from server" ;
	char clientMessage[MAX_SIZE];
	int readval;
	int sendval;
	int row;
	int col;
 	Parameter_t allparam;
 	int childs=3;
 	pid_t pid[childs];
 	int i;
 	Sender_t parameter;

 	/********entry section *****/
	while(sem_wait(&semlockp) == -1)
	{
		if(errno != EINTR)
		{
			fprintf(stderr, "Thread failed to lock semaphore\n");
			return NULL;
		}
	}
	/******** Start of "Critical Section"  ***********/
	
	
	if(readForData(newsock, &allparam ,sizeof(allparam)) == -1)
	{
		perror("Failed!");
		exit(ZERO);
	}

	for (i= ZERO; i < childs; i++)
 	{
 		if((pid[i] = fork()) == -1)
 			exit(ZERO);
 		if(pid[i] == ZERO) // cocuk ise
 			break;
 	}

 	switch (i)
 	{
 		case 0: 
 			//generate 
 			generate(allparam.col, allparam.row,newsock);
 			break;
 		case 1:
 			//solve
 			solve(allparam.row, allparam.col);
 			break;
 		case 2:
 			//verify
 			verify(allparam.row, allparam.col, newsock);
 			break;
 		
 	}

	/************* Exit section  **********/
	if(sem_post(&semlockp) == -1) 
		fprintf(stderr, "Thread failed to unlock semaphore\n");

	/************ remainder section **************/

}

void generate(int col, int row,int socketfd)
{
	int i=ZERO, j=ZERO;
	int shID;
	double matrixA[DIMENSION*DIMENSION];
	double matrixB[DIMENSION*DIMENSION];
	Sender_t *parameter;
	Sender_t yeter;
	int sid;
	Sender_t *son;

	//shared memory olusturma
	if((shID= shmget(124, sizeof(Sender_t*), S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}
	//attaching 
	parameter=(Sender_t *)shmat(shID, NULL, ZERO);

	// generateden gelen shared memoryi okuma 
	if((sid = shmget(436, sizeof(Sender_t*), PERM)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}

	if((son = (Sender_t *)shmat(sid,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}
	//printf("%f\n",son->errorNorm );

	srand(getpid());

	for(i = 0; i < row; i++)
	{
		for(j= 0; j < col; j++)
		{
			matrixA[i*col +j] = rand() % 100 + 1;
		}
	}
	
	for(i=0;i<row;i++)
  	{
	    for(j=0;j<col;j++)
	    {
	     	parameter->matrisA[i][j]=matrixA[i*col+j];
	     	yeter.matrisA[i][j]=parameter->matrisA[i][j];
	    }
  	}

	for(i = 0; i < row; i++)
	{
		for(j= 0; j < ONE ; j++)
		{
			matrixB[i*col +j] = rand() % 100 + 1;
		}
	}
  	for(i=0;i<row;i++)
  	{
	    for(j=0;j<ONE;j++)
	    {
	      	parameter->matrisB[i][j]= matrixB[i*col+j];
	     	yeter.matrisB[i][j]=parameter->matrisB[i][j];

	    }
  	}

  	yeter.errorNorm= son->errorNorm;

  	writeForData(socketfd, &yeter,sizeof(yeter));

 }

// A.x= B lineer denklemini çözen fonksiyon
void solve(int row,int col)
{
	double det;
	int i,j,k;
	int shredmem;
	double *matrixA;
	double *matrixB;
	Sender_t *mat;
	double ters[DIMENSION][DIMENSION];
	double transpose[DIMENSION][DIMENSION];
	double result[DIMENSION][DIMENSION];
	Forverify_t *veri;
	int shaID;
	FILE *f;
	char buf[MAX_SIZE];
	P2toP3_t *sendingVerify;
	Sender_t yeter;
	int sham;

	sprintf(buf,"logs/Ser_%lu.log",syscall(SYS_gettid));
	f= fopen(buf,"a+");



	//shared memory olusturma, inverse matris icin trasnpoze ile baglantı
	if((shaID= shmget(144, sizeof(Forverify_t*), S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}
	

	if((veri = (Forverify_t *)shmat(shaID,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}


	// generateden gelen shared memoryi okuma 
	if((shredmem = shmget(124, sizeof(Sender_t*), PERM)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}

	if((mat = (Sender_t *)shmat(shredmem,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}

		//shared memory olusturma, verify ile baglantı için
	if((sham= shmget(5000, sizeof(P2toP3_t *), S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}

	//attaching 
	//sendingVerify=(P2toP3_t *)shmat(sham, NULL, ZERO);	

	if((sendingVerify = (P2toP3_t *)shmat(sham,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}

	for (i = 0; i < row; ++i)
	{
		for (j = 0; j < col; ++j)
		{
	     	yeter.matrisA[i][j]=mat->matrisA[i][j];
	     	sendingVerify->matrisA[i][j]=mat->matrisA[i][j];
		}
	} 

	for(i=0;i<row;i++)
  	{
	    for(j=0;j<ONE;j++)
	    {
	      	yeter.matrisB[i][j]= mat->matrisB[i][j];
	      	sendingVerify->matrisB[i][j]=mat->matrisB[i][j];
	    }
  	}
 	//---> burdan yararlanılmıstır:: https://www.programiz.com/c-programming/examples/matrix-transpose
 	// A matrisinin transpozesi 
	for(i=0; i<row; ++i)
    {
    	for(j=0; j<col; ++j)
	    {
	        transpose[j][i] = yeter.matrisA[i][j];
	    }
	}

	// A transpoze * A işlemini yapar
	for(i=0; i<col; ++i)
	{
		for(j=0; j<col; ++j)
		{
			for(k=0; k< row; ++k)
			{
				ters[i][j]+= transpose[i][k]*yeter.matrisA[k][j];
			}
		}
	}
	det=determinant(ters,col);


	if(det== ZERO)
	{
		printf("Pseudo Inverse doesn't exits! If row<col, then the inverse of AT*A does not exist . \n");
		exit(ZERO);
	}
	else
	{
		cofactor(ters, col);
	   // Initializing all elements of result matrix to 0
		for(i=0; i<row; ++i)
		{
			for(j=0; j<1; ++j)
			{
				yeter.X[i][j] = 0.0;
			}
		}

		// Multiplying matrices AT* A inverse and AT 
		for(i=0; i<col; ++i)
		{
			for(j=0; j<row; ++j)
			{
				for(k=0; k< col; ++k)
				{
					result[i][j]+=veri->inverse[i][k]*transpose[k][j];
				}
			}
		}

		// Multipliying matrices result * B
		for(i=0; i<col; ++i)
		{
			for(j=0; j<1; ++j)
			{
				for(k=0; k< row; ++k)
				{
					yeter.X[i][j]+=result[i][k]*yeter.matrisB[k][j];
					mat->X[i][j]= yeter.X[i][j];
					sendingVerify->X[i][j]=yeter.X[i][j];
				}
			}
		}

		fprintf(f, "A= [ ");
		for(i=0;i<row;i++)
	  	{
		    for(j=0;j<col;j++)
		    {
		      	fprintf(f,"%.1f ",mat->matrisA[i][j]);
		    }
			if( i != row-1)
		   		fprintf(f,"; ");
	  	}
	  	fprintf(f, "]\n\n");

	  	fprintf(f,"=========================================================\n\n");

	  	fprintf(f, "B= [ ");
	  	for(i=0;i<row;i++)
	  	{
		    for(j=0;j<ONE;j++)
		    {
		      	fprintf(f,"%.1f ",mat->matrisB[i][j]);
		    }
		    if(i != row-1)
		   		fprintf(f,"; ");
	  	}
	  	fprintf(f, "]\n\n");

	  	fprintf(f,"=========================================================\n\n");


		// Displaying the result
		fprintf(f,"x= [ ");
		for(i=0; i<col; ++i)
		{
			for(j=0; j<1; ++j)
			{
				fprintf(f,"%f ", mat->X[i][j]);
			}
			if(i != col-1)
		   		fprintf(f,"; ");
		}
	  	fprintf(f, "]\n\n");

	} 

	fclose(f);

}

void verify(int row, int col, int socketfd)
{
	int sharedId;
	P2toP3_t *alici;
	int i, j, k;
	double carp[20][1];
	double e[20][1];
	double eT[1][20];
	double result;
	double son;
	int ss;
	Sender_t *sent;

	// solve'dan gelen shared memoryi okuma 
	if((sharedId = shmget(5000, sizeof(P2toP3_t *), PERM)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}

	if((alici = (P2toP3_t *)shmat(sharedId,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}

	// Multiplying matrices A and x 
	for(i=0; i<row; ++i)
	{
		for(j=0; j<1; ++j)
		{
			for(k=0; k< col; ++k)
			{
				carp[i][j]+=alici->matrisA[i][k]*alici->X[k][j];
			}
		}
	}
	//calculate Ax- B
	for(i=0; i<row; i++)
	{
       for(j=0;j<1;j++)
            e[i][j]=carp[i][j]-alici->matrisB[i][j];
	}

	// Finding the transpose of matrix e
    for(i=0; i<1; ++i)
    {
	    for(j=0; j< row; ++j)
	    {
	        eT[j][i] = e[i][j];
	    }
	}

	// Multiplying matrices eT and e
	for(i=0; i<1; ++i)
	{
		for(j=0; j<1; ++j)
		{
			for(k=0; k< col; ++k)
			{
				son+=eT[i][k]*e[k][j];
			}
		}
	}

	//shared memory olusturma, verify ile baglantı için
	if((ss= shmget(436, sizeof(Sender_t *), S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}

	
	if((sent = (Sender_t *)shmat(ss,NULL ,ZERO)) ==(void*)-1)
	{
		perror("attaching error");
		exit(ONE);
	}
	result = sqrt(abs(son));
	sent->errorNorm=result;
	//printf( "Error norm: %f\n",result);
	

	
}

//Bu fonksiyon kitaptan alınmıştır.
int readForData(int s, void *buf, int n)
{
	int byteCount= ZERO;
	int byteRead= ZERO;

	while(byteCount < n)
	{
		if((byteRead = read(s, buf, n- byteCount)) > ZERO)
		{
			byteCount += byteRead;
			buf += byteRead;
		}
		else if(byteRead < ZERO)
			return -1;
	}
	return byteCount;
}

int writeForData(int s, void *buf, int n)
{
	int byteCount= ZERO;
	int byteWrite= ZERO;

	while(byteCount < n)
	{
		if((byteWrite = write(s, buf, n- byteCount)) > ZERO)
		{
			byteCount += byteWrite;
			buf += byteWrite;
		}
		else if(byteWrite < ZERO)
			return -1;
	}

	return byteCount;

}


void sigintHandler(int pid)
{
	FILE *file;
	int num;
	file=fopen("id.txt","r");
	if(file)
	{
		while(fscanf(file,"%d\n",&num)> ZERO)
			kill(num,SIGKILL);
	    fclose(file);
	}
	
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

/* Bu siteden alınmıştır .. 
http://www.ccodechamp.com/c-program-to-find-inverse-of-matrix/
/*Finding transpose of cofactor of matrix*/ 
void transpose(double matrix[DIMENSION][DIMENSION],
				double matrix_cofactor[DIMENSION][DIMENSION],double size)
{
	
	int i,j;
	float m_transpose[DIMENSION][DIMENSION],d;
	int a=0;
	int b=0;
	Forverify_t *ver;
	double m_inverse[DIMENSION][DIMENSION];
	int sha;

	// inverse matrisi shared memorye yazmak için
	if((sha = shmget(144, sizeof(Forverify_t*), PERM)) == -1)
	{
		perror("shmget");
		exit(ONE);
	}
	ver=(Forverify_t *)shmat(sha, NULL, ZERO);		

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
			 ver->inverse[i][j]= m_inverse[i][j];
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

