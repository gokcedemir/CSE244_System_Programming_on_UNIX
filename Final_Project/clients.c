/* 
 * File:   server.c
 * Author: Gökçe Demir - 141044067
 *	System_Programming Final_Project
 * Created on May 22, 2017, 10:43 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> //write
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
#include <netinet/in.h> // for socket
#include <netdb.h>      // for socket
#include <pthread.h>
#include <arpa/inet.h> //inet_addr
#include <semaphore.h>
#include <sys/syscall.h> // thread id


#define ZERO 0
#define ONE 1
#define SIZE_ARR 255
#define MAXHOSTNAME 255
#define MAX_SIZE 1024


typedef struct Parameter_t{
   int row;		//matrix size icin thread fonksiyonunu gonderme
   int col;
   unsigned short portnumber;

}Parameter_t;

typedef struct Receiver_t
{
	double matrisA[20][20];
	double matrisB[20][1];
	double X[20][1];
	double errorNorm;
}Receiver_t;

/***Global Variables***/
FILE *fptr;  // ctrl-c icin id tutan file
FILE *f; // server yoksa clientta yok --
int size=0;
sem_t semlockp;


// Bu fonksiyon kitaptan alınmıştır.
int writeForData(int s, void *buf, int n);
int readForData(int s, void *buf, int n);



//sinyal yakalayan fonksiyon
void sigintHandler(int sig);

/*thread function*/
void *callSocket(void *arg);

int main(int argc, char** argv)
{

	unlink("ff.txt");	
	fptr=fopen("id.txt","a+");
	fprintf(fptr,"%d\n",getpid());
    fclose(fptr);
    int error;
    pthread_t *tid;
    int i=0;
    Parameter_t allparameter; //struct arrayim
    mkdir("logs", 0777);
    FILE *fx;
    FILE *ff;
    float x;
    float sum;
    double standartdev;
    float kare;

	//usage kontrolu yapilir
	if(argc != 5)
	{
		fprintf(stderr,"Usage: %s <#of columns of A, m><#of rows of A, p>,"
			"<#of clients, q>, <portnumber>\n",argv[0]);
		exit(ZERO);

	}

	int column= atoi(argv[1]);
	int row=atoi(argv[2]);
	int numOfClient= atoi(argv[3]); // argüman q
	unsigned short portnum=atoi(argv[4]); // port numarasını client'in argümanı olarak alıyorum
    signal(SIGINT, sigintHandler);

	if(!(f=fopen("control.txt","r")))  //server önce başlatılmıs mı onu kontrol etmek icin olusturulan .txt
	{
		printf("Firstly, server should be run!\n");
		exit(ZERO);
	}
 	
	allparameter.row= row;
	allparameter.col = column;
	allparameter.portnumber= portnum;

	tid=(pthread_t *)calloc(numOfClient,sizeof(pthread_t));
	if(tid==NULL)
	{
		perror("Failed to allocate memory for thread IDs");
		return ONE;
	}


	if(sem_init(&semlockp, ZERO, ONE) == -1)
	{
		perror("Failed to initialize semaphore");
		return ONE;
	}

	for(i= ZERO; i<numOfClient; i++)
	{
    	if(error=pthread_create(&tid[i],NULL, callSocket, &allparameter))
        {
            fprintf(stderr,"Failed to create thread: %s\n",strerror(error));
            return ONE;
        }
	}


	for(i =0 ; i< numOfClient; i++)
	{
		if(error= pthread_join(tid[i], NULL))
		{
			fprintf(stderr,"Failed to join thread: %s\n",strerror(error));
			return ONE;
		}
	}

	fx=fopen("logs/xi.log", "a+");
	ff= fopen("logs/clients.log","a+");
	
	while(fscanf(fx,"%f\n",&x) > ZERO)
	{
		kare+=pow(x,2);
		sum+=x;		
	}

	double mean;
	mean= sum/numOfClient;
	fprintf(ff,"Mean: %f\n",mean);
	
	
	standartdev=sqrt(((kare)-(numOfClient*mean*mean))/(numOfClient-1));
	fprintf(ff,"Standart deviation: %.40f\n", standartdev);

	fclose(fptr);
	fclose(fx);

	return ZERO;
}

void *callSocket(void *arg)
{
	char message[MAX_SIZE];
	char serverMess[MAX_SIZE];
 	char myname[MAXHOSTNAME+ONE];
    int socketfd;
	struct sockaddr_in sa;
	struct hostent *hp;
    Parameter_t allparam=*(Parameter_t*)arg;
    unsigned short portnum = allparam.portnumber;
    int row= allparam.row;
    int col= allparam.col;
    char buffer[MAX_SIZE];
    int readval;
    int sendval;
    Receiver_t param;
    int i=0, j=0;
    FILE *fptr;
    char filename[MAX_SIZE];
    double mean;
    double xi;
    FILE *fx;
   

    clock_t start = clock();

    sprintf(filename,"logs/Cli_%lu.log", syscall(SYS_gettid)) ;
    fptr=fopen(filename,"a+");

    fx= fopen("logs/xi.log","a+");

	gethostname(myname,MAXHOSTNAME);  //biz kimiz?
	
	//adres bilgimiz
	if ((hp= gethostbyname(myname)) == NULL)
    {
		errno= ECONNREFUSED;
		exit(ONE);
	}
	memset(&sa, ZERO, sizeof(sa));  //adres temizleme

  	sa.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhosta baglanırız
	sa.sin_family=hp->h_addrtype;  //host adresi
	sa.sin_port= htons((u_short)portnum);

	//socket olusturma --> bu kısımlar kitaptan esinlenerek yapılmıştır
	if ((socketfd= socket(hp->h_addrtype,SOCK_STREAM,0)) < 0)
		exit(ONE);

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

	
	if (connect(socketfd,(struct sockaddr *)&sa,sizeof sa) < 0)
	{ 
		close(socketfd);
		exit(ONE);
	} 


	writeForData(socketfd,&allparam,sizeof(allparam));

	readForData(socketfd, &param, sizeof(param));


	fprintf(fptr, "A= [ ");
	for(i=0;i<row;i++)
  	{
	    for(j=0;j<col;j++)
	    {
	      	fprintf(fptr,"%.1f ",param.matrisA[i][j]);
	    }
	    if(i != row-1)
	   		fprintf(fptr,"; ");
  	}
  	fprintf(fptr, "]\n\n");

  	fprintf(fptr,"=========================================================\n\n");

  	fprintf(fptr, "B= [ ");
  	for(i=0;i<row;i++)
  	{
	    for(j=0;j<ONE;j++)
	    {
	      	fprintf(fptr,"%.1f ",param.matrisB[i][j]);
	    }
 		if(i != row-1)
	   		fprintf(fptr,"; "); 
 	}
  	fprintf(fptr,"]\n\n");

	fprintf(fptr,"=========================================================\n\n");

	fprintf(fptr, "Error norm: %f\n", param.errorNorm );

	xi= ((double)clock() - start) / CLOCKS_PER_SEC;

	fprintf(fx,"%f\n", xi);

	/************* Exit section  **********/
	if(sem_post(&semlockp) == -1) 
		fprintf(stderr, "Thread failed to unlock semaphore\n");

	/************ remainder section **************/

	fclose(fptr);
	fclose(fx);

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

//Bu fonksiyon kitaptan alınmıştır..
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
	int num[SIZE_ARR];
	int mypid=(int)getpid();
	fptr=fopen("id.txt","r");
	int i=0;
	while(fscanf(fptr,"%d\n",&num[i])>ZERO)
	{
		i++;
		++size;
	}
	fclose(fptr);

	fptr=fopen("id.txt","w");
	i=0;
	while(i<size)
	{
		if(num[i]!=mypid)
			fprintf(fptr, "%d\n",num[i] );
		i++;
	}
	fclose(fptr);

    printf("\nKilling process %d..\n",getpid());
    exit(0);
}
