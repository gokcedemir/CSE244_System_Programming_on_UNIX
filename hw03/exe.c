/* 
 * File:   exe.c
 * Author: Gökçe Demir 
 * Student Id: 141044067
 * System_Programming_HW03
 * Created on March 15, 2017, 10:23 AM
 */

/******************************************************************************/
/* Bu program verilen directory altında bir string arar. Bunu paralel		  */
/* paralel programlama ile sağladım. Directory icindeki directorylerde        */
/* recursive cagri yaparak alt klasorlere ulastım. Processler arasindaki      */
/* haberlesmeyi de pipe ile yaptim. Toplam bulunan string sayisini ve stringin*/
/* bulundugu satir ve sutunu "log.txt" isimli dosyaya pipe'dan okuyarak yazar.*/
/*					./exe string dirname				  					  */
/******************************************************************************/


#include <limits.h> /* PATH_MAX defined in */
#include <dirent.h> // DIR* 
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  //for pipe
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>   //for mkfifo function

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


/*  Bu fonksiyon path'in directory olup olmadığına bakar.    
    Return Degeri: Eger directory ise 1, değil ise 0 döndürür.   */
int isDirectory(const char *directName);

/*Bu fonksiyon directoryleri tarar. Directory icinde directory durumunda     */
/* recursive cagrı yaparak directoryleri acar. Dosyalar da ise fork yapar.    */ 
/* RETURN DEGERİ: Eger basarı ise 0, aksi halde farklı bi deger dondurur.     */
int listDirectory(const char *word, const char *directname);

/*Bulmak istedigimiz stringi dosyada arar. Return degeri integer bir degerdir.*/
int searchWordInFile(const char *word, const char *folderName,
					const char *filename, int *fildes);

/*Verilen directory'in icindeki toplam klasor ve dosya sayisini bulur.      */
int calculateNumOfFileAndDir(DIR * dirp, const char * directname,
        int * totalDir, int * totalFile);

int outputFileDesc;  //bulunan stringi yazdıgım dosyanin file descriptoru


int main(int argc, char** argv)
{
    unlink(LOG_FILE);   //daha onceden olusturulmus log.txt'yi siler 
	FILE *fileP;
    char ch;
    int numOfTotalString=0;
    char *target= argv[1];   //string
    char *folder = argv[2];   //directory name
    //usage kontrol
    if(argc !=3)
    {
        //Ekrana olası durumda hata basma
        fprintf(stderr, "Usage: %s which_word which_directname\n",argv[0]);
        exit(ONE);
    }
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
    int pipeArray[PATH_MAX][2];
    
    int total=0;       //file+dir
    int totalFile=0;  //toplam file sayisi
    int totalDir=0;   //toplam directory sayisi
    int fileCounter=0;
    int check;
    
    int i=0, j=0, rvalue=0;
    
  
    if((dirp = opendir(directname)) == NULL)
    {
        perror("Failed to open directory");
        return 1;
    }
	/* Toplam file ve directory sayisini hesaplama */
    calculateNumOfFileAndDir(dirp, directname, &totalDir, &totalFile);
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
            
            //fifo oluşturma
            //mkfifo(fifoName[dirCounter],FIFO_PERM);
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
        /* Eger pathimiz file ise cocuk process ile fonksiyonumuzu cagırarak 
         bulmak istedigimiz stringi ararız.  */
        else if(checkDirectory ==ZERO && check)
        {
			
            if(pipe(pipeArray[count])== -1)//create pipe
			{
				perror("Pipe failed.\n");
			}  
			childPid=fork();   //fork yapilir
            
            if(childPid < ZERO)
            {
                printf("Fork failed.\n");
                exit(FALSE);
            }
            else if(childPid == ZERO) //cocuk process pipe'a yazar  
            {
                strcpy(fileName, direntPtr->d_name);
                searchWordInFile(word,newPathName,fileName,pipeArray[count]);
                exit(ZERO);
            }
            else
            {
                wait(ZERO);
                ++fileCounter;
			  	++count;
            }
        }
    }
    
   
    /*Parent processin pipe'dan okumasi*/
    if(childPid > ZERO )
    {
        // pipe'dan okuyarak olusturulan log dosyasına yazma
        j=ZERO;
		while(j < totalFile)
        {
            char buf[PATH_MAX]= "";
            close(pipeArray[j][ONE]);
            int readbyte = read(pipeArray[j][ZERO],buf,PATH_MAX);
            write(outputFileDesc, buf, readbyte);
            close(pipeArray[j][ZERO]);  
            j++;
        }
    }
 
    closedir(dirp);
}

int searchWordInFile(const char *word, const char *folderName,
                    const char *filename,int fildes[])
 {
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
    char buffer[PATH_MAX];
      
   
    // check file 
    if((iPath = open(folderName, READ_FLAGS))== -1)
    {
        perror("Failed to open input file");
        return 1;
    }
    
    for (i = 0; (x = read(iPath,readByte,1))!= 0; i++);
    sizeOfArray=i;  // dosya boyutu kadar array ayıracagiz
    
    line = (char*) (calloc(sizeOfArray, sizeof (char)));
    
    iPath = open(folderName,READ_FLAGS);
    int k = 0;
    x=read(iPath, line,sizeOfArray);  //line isimli arrayi doldurma
    while (k < sizeOfArray)
    {
        if(line[k]=='\n')
        {
            ++lineCount;
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
                    //bulduklarını buffera yazar
                    sprintf(buffer, "%s: [%d,%d] %s first character is found.\n",
                            filename,lineCount, columnNum, word);
                    //bufferi pipe'a yazar
                    write(fildes[1],buffer,strlen(buffer));
                    ++frequencyOfStr;
                    j=0;
                    break;
                }            
            }
            j=0;
        }
        ++k;
    }
    
    free(line);  //alinan yeri geri verme
    close(iPath);  //dosya kapama
    return 0;

}
