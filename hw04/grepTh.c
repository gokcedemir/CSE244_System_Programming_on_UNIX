/* 
 * File:   grepTh.c
 * Author: Gökçe Demir
 * Student Id: 141044067
 * System_Programming_HW04
 * Created on April 24, 2017, 10:56 PM
 */

/******************************************************************************/
/*Bu program arguman olarak aldıgı dırectory altındaki tum directory ve dosyaları
 tarayarak yine arguman olarak aldıgı stringi arar. Buldugu yerin koordinatlarını
 ve dosya isimlerini log.txt isimli dosyaya basar. Bu islemleri yaparken
 directory icin process olusturdum, eger file ise de threadler olusturdum.*/

/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> /* PATH_MAX defined in */
#include <dirent.h> // DIR* 
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>  //for pipe
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>   //for mkfifo function
#include <sys/time.h>
#include<signal.h>  
#include<semaphore.h>

/*MACROS*/
#define TRUE 1
#define FALSE 0
#define READ_FLAGS O_RDONLY  //file macro
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define ZERO 0
#define ONE 1
#define LOG_FILE "log.txt"
#define FIFO_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

sem_t mainSema;

typedef struct Parameter_t{
    char word[PATH_MAX];
    char folderName[PATH_MAX];
    char filename[PATH_MAX];
}Parameter_t;

/*  Bu fonksiyon path'in directory olup olmadığına bakar.    
    Return Degeri: Eger directory ise 1, değil ise 0 döndürür.   */
int isDirectory(const char *directName);

/*Bu fonksiyon directoryleri tarar. Directory icinde directory durumunda     */
/* recursive cagrı yaparak directoryleri acar. Dosyalar da ise fork yapar.    */ 
/* RETURN DEGERİ: Eger basarı ise 0, aksi halde farklı bi deger dondurur.     */
int listDirectory(const char *word, const char *directname);

/*Bulmak istedigimiz stringi dosyada arar. Return degeri integer bir degerdir.*/
void *searchWordInFile(void *arg);

/*Verilen directory'in icindeki toplam klasor ve dosya sayisini bulur.      */
int calculateNumOfFileAndDir(DIR * dirp, const char * directname,
        int * totalDir, int * totalFile);

int outputFileDesc;  //bulunan stringi yazdıgım dosyanin file descriptoru

/*Sinyal yakalama fonksiyonu */
void sig_handler(int signo)
{
	if(signo == SIGINT || signo == SIGUSR1 || signo == SIGKILL || signo == SIGSTOP)
	    printf("Exit condition: due to signal no#%d\n",signo);
	exit(ZERO);
}


int main(int argc, char** argv)
{
    struct timespec start_time, end_time;
    double duration = 0.0;
    unlink(LOG_FILE);   //daha onceden olusturulmus log.txt'yi siler 
    unlink("totalDir.txt");
    unlink("totalFile.txt");
    unlink("totalLine.txt");
    FILE *fileP;
    char ch;
    int numOfTotalString=0;
    char *target= argv[1];   //string
    char *folder = argv[2];   //directory name
    int num=0, num1=0, num2=0;
    FILE *fptr;
    FILE *file;
    FILE *fileLine;
    int numOfDir=0, i=0;
    int numOfFile=0;
    int numOfLine=0;
	int rval;
	int fileArr[PATH_MAX];
    
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    //usage kontrol
    if(argc !=3)
    {
        //Ekrana olası durumda hata basma
        fprintf(stderr, "Usage: %s which_word which_directname\n",argv[0]);
        exit(ONE);
    }
    
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGSTOP, sig_handler);	
	


    outputFileDesc = open(LOG_FILE, WRITE_FLAGS, FIFO_PERM);
    listDirectory(target, folder);
    close(outputFileDesc);
	
    fileP=fopen(LOG_FILE,"a+");
    do{
        ch = getc(fileP);
        if(ch== '\n')
            ++numOfTotalString;
    } while (ch != EOF);
    fprintf(fileP,"%d %s were found in total.", numOfTotalString, target);    
    
    fptr=fopen("totalFile.txt","r");
    file=fopen("totalDir.txt","r");
    fileLine=fopen("totalLine.txt","r");
    
	int z=0;
    while(fscanf(fptr,"%d\n",&num) > ZERO)
	{
        numOfFile+=num;
		fileArr[z]=num;
		z++;
	}
	fclose(fptr);
    while(fscanf(file,"%d\n",&num1) > ZERO)
        numOfDir+=num1;
    while(fscanf(fileLine,"%d\n",&num2)>0)
        numOfLine+=num2;
    printf("Total number of strings found :  %d\n",numOfTotalString);
    printf("Number of directories searched:  %d\n", numOfDir+1);
    printf("Number of files searched:  %d\n", numOfFile);
    printf("Number of lines searched:  %d\n",numOfLine);
	printf("Number of search threads created:   %d\n",numOfFile);
	
    fptr=fopen("totalFile.txt","r");
	printf("Number of cascade threads created:  ");
	while(fscanf(fptr,"%d\n",&num)> ZERO)
	{
		if(num !=ZERO)
			printf("%d ",num);
	}
	printf("\n");
	for(i = 1; i < (numOfFile+numOfDir); ++i)
    {
       if(fileArr[0] < fileArr[i])
           fileArr[0] = fileArr[i];
    }
    printf("Max # of threads running concurrently: %d\n", fileArr[0]);

    fclose(fptr);
    fclose(file);
    fclose(fileLine);
    
   
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time); // get final time-stamp
	duration= (double)(end_time.tv_sec - start_time.tv_sec) * 1.0e9 +
              (double)(end_time.tv_nsec - start_time.tv_nsec);
	duration*= 0.000001;  //convert to ns to ms
    printf("Total run time, in milliseconds:  %f\n",duration);

	printf("sleeping 3 seconds for signal ..\n");    
	sleep(3);   

	printf("Exit condition: normal\n");
	unlink("totalDir.txt");
    unlink("totalFile.txt");
    unlink("totalLine.txt");
    return ZERO;
    
}

int isDirectory(const char *directName)
{
    struct stat statbuf;
    if (stat(directName, &statbuf) == -1)
        return ZERO;
    else 
        return S_ISDIR(statbuf.st_mode);
}

int calculateNumOfFileAndDir(DIR * dirp, const char * directname,
                            int * totalDir, int * totalFile)
{
    struct dirent *direntPtr;
    int sizeOfDirname=0;
    int checkDirectory=0;
    char newPathName[PATH_MAX];
    int check;
    FILE *fptr;
    FILE *file;
    
    fptr=fopen("totalFile.txt","a+");
    file=fopen("totalDir.txt","a+");
    
    while((direntPtr = readdir(dirp)) != NULL)
    {
        strcpy(newPathName, directname);
        char *str = direntPtr->d_name;
        sizeOfDirname = strlen(str);
        
        /*directory kontrolu yapilir, eger  "." , ".." veya "~" 
         ile biten directory ise gozardı edilir  */
        check = strcmp(direntPtr->d_name,".")!= ZERO &&
           strcmp(direntPtr->d_name,"..")!=ZERO &&
           direntPtr->d_name[sizeOfDirname-1]!='~';
        if(check)
        {
            strcpy(newPathName, directname);
            strcat(newPathName,"/");
            strcat(newPathName, direntPtr->d_name);
        }
        checkDirectory=isDirectory(newPathName);
        /*Eger yeni pathimiz directory ise directory sayisi arttirilir.  */
        if(check && checkDirectory!=ZERO)
            ++(*totalDir);
       /* Eger pathimiz file ise file sayisi arttirilir.    */
        else if(checkDirectory ==ZERO && check)
            ++(*totalFile);
    }
    fprintf(fptr,"%d\n",*totalFile);
    fprintf(file,"%d\n",*totalDir);
   
    fclose(fptr);
    fclose(file);
    /*Reset the position of the directory stream.*/
    rewinddir(dirp);
    return ZERO;
}

int listDirectory(const char *word, const char *directname)
{
    struct dirent *direntPtr;
    DIR *dirp;
    pid_t childPid;
    char newPathName[PATH_MAX];  //new path name
    int checkDirectory =0;    
    int sizeOfDirname =0;   
    char fileName[PATH_MAX];
    
    int total=0;       //file+dir
    int totalFile=0;  //toplam file sayisi
    int totalDir=0;   //toplam directory sayisi
    int fileCounter=0;
    int check;
    int i=0, j=0, rvalue=0;
    Parameter_t *allparameter; //struct arrayim
    pthread_t *tid;
    int error;
    int counter=0;


    if((dirp = opendir(directname)) == NULL)
    {
        perror("Failed to open directory");
        exit(ZERO);
    }
	/* Toplam file ve directory sayisini hesaplama */
    calculateNumOfFileAndDir(dirp, directname, &totalDir, &totalFile);
	allparameter=(Parameter_t*)malloc(sizeof(Parameter_t)*totalFile);
	tid=(pthread_t*)malloc(sizeof(pthread_t)*totalFile);
    int count=0;
    while((direntPtr = readdir(dirp)) != NULL)
    {
		
        strcpy(newPathName, directname);
        char *str = direntPtr->d_name;
        sizeOfDirname = strlen(str);
        
        /*directory kontrolu yapilir, eger  "." , ".." veya "~" 
         ile biten directory ise gozardı edilir  */
        check = strcmp(direntPtr->d_name,".")!= ZERO &&
           strcmp(direntPtr->d_name,"..")!=ZERO &&
           direntPtr->d_name[sizeOfDirname-1]!='~';
        if(check)
        {
            strcpy(newPathName, directname);
            strcat(newPathName,"/");
            strcat(newPathName, direntPtr->d_name);
        }
        checkDirectory=isDirectory(newPathName);
        /*Eger yeni pathimiz directory ise child process olusturularak recursive 
          cagrı yapılır ve directory icinde tarama yapılır.*/
        if(check && checkDirectory!=ZERO)
        {
            childPid= fork();  //yeni process oluşturma
            if(childPid < ZERO)  /*eger process olusmaz ise*/
            {
                printf("Fork failed.\n");
                exit(FALSE);
            }
            else if(childPid == ZERO) /* child process*/
            {
                //recursive cagrı
                listDirectory( word,newPathName);
                exit(ZERO);
            }
            else
                wait(ZERO);
        }
        /* Eger pathimiz file ise cocuk process ile thread olusturarak 
         bulmak istedigimiz stringi ararız.  */
        else if(checkDirectory ==ZERO && check)
        {
                strcpy(fileName, direntPtr->d_name);
                strcpy(allparameter[counter].word,word);
                strcpy(allparameter[counter].folderName,newPathName);
                strcpy(allparameter[counter].filename,fileName);

                if(error=pthread_create(&tid[counter],NULL, searchWordInFile, &allparameter[counter]))
                {
                    fprintf(stderr,"Failed to create thread: %s\n",strerror(error));
                    exit(ZERO);
                }
               ++counter; 
        }
    }
    
    int a;
    for (a = 0; a < counter; ++a)
    {
        if(error = pthread_join(tid[a],NULL))
        {
            fprintf(stderr,"Failed to join thread: %s\n",strerror(error));
            exit(ZERO);
        }
    }

    free(allparameter);
	free(tid);
	tid=NULL;
	allparameter=NULL;
    closedir(dirp);

	return 0;
}

void *searchWordInFile(void *arg)
 {
	sem_init(&mainSema,ZERO,ONE);
	sem_wait(&mainSema);
    int lineCount = ONE;  // satir sayisi
    int colCount = ONE;   //sutun sayisi
    char *line = "";      // dosyayi okuduktan sonra doldurulan array
    int i = ZERO, j = ZERO,
        frequencyOfStr = ZERO; // toplam string sayisi
    int iPath= -1;  // file descriptors
    char readByte[ONE]; // buffer
    int sizeOfArray=ZERO;
    int index=ZERO;
    int columnNum=ONE;
    int x=ZERO;

    FILE *fileLine;
    int numOfLines=ZERO;
    int tt=ZERO;
    Parameter_t allparam=*(Parameter_t*)arg;
	
	char word[PATH_MAX];
	char folderName[PATH_MAX];
	char filename[PATH_MAX];

	strcpy(word,allparam.word);
	strcpy(folderName,allparam.folderName);
	strcpy(filename,allparam.filename);
 
    FILE *ff;
	
	
    ff=fopen(LOG_FILE,"a+");
    // check file 
    if((iPath = open(folderName, READ_FLAGS))== -1)
    {
        perror("Failed to open input file");
        exit(ZERO);
    }
    fileLine=fopen("totalLine.txt","a+");
    
    for (i = 0; (x = read(iPath,readByte,1))!= 0; i++);
    sizeOfArray=i;  // dosya boyutu kadar array ayıracagiz
    
    line = (char*)(calloc(sizeOfArray,sizeof(char)));
    
    iPath = open(folderName,READ_FLAGS);
    int k = 0;
    x=read(iPath, line,sizeOfArray);  //line isimli arrayi doldurma
    while (k < sizeOfArray)
    {
        if(line[k]=='\n')
        {
            ++lineCount;
            ++numOfLines;
            colCount = 1;
            
        }
        else
            ++colCount;
        index=k;
        if(line[index] == word[j])
        {
            columnNum= colCount-1;
             ++j;
            ++index;
            while(line[index]=='\n' || line[index]=='\t' || line[index]==' ')
            {
                ++index;
            }                       
            while( line[index]== word[j])
            {
                ++index;
                ++j;
                while(line[index]=='\n' || line[index]=='\t' || line[index]==' ')
                {
                    ++index;
                }  
                if(j== strlen(word))
                {
                    fprintf(ff, "ProcessID: %d -ThreadID: %u %s: [%d,%d] %s "
                                 "first character is found.\n",(int)getpid(),
                                (unsigned int)pthread_self(),filename,lineCount, 
                                columnNum, word);
                    ++frequencyOfStr;
                    j=0;
                    break;
                }            
            }
            j=0;
        }
        ++k;

    }
    fprintf(fileLine,"%d\n",numOfLines);

    free(line);  //alinan yeri geri verme
	line=NULL;
    close(iPath);  //dosya kapama
    fclose(fileLine);
    fclose(ff);
	sem_post(&mainSema);  //semaphoreee
}
