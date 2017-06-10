/* 
 * File:   listdir.c
 * Author: Gökçe Demir / 141044067
 *
 * Created on March 5, 2017, 10:13 PM
 */


/******************************************************************************/
/* Bu program verilen directory icinde string arıyor. Bulunan stringlerin     */
/* buldugunu column ve rowu bir de bulundugu dosyanın adını log.txt isimli    */
/* dosya yazar.                                                               */
/******************************************************************************/


#include <stdio.h>
#include <limits.h> /* PATH_MAX defined in */
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h> // DIR* 
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

/*MACROS*/
#define TRUE 1
#define FALSE 0
#define READ_FLAGS O_RDONLY  //file macro
#define ZERO 0
#define ONE 1


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
int searchWordInFile(const char *word, const char *folderName, const char *filename);

int main(int argc, char** argv)
{
	unlink("log.txt");
    FILE *fileP;
    char ch;
    int numOfTotalString=0;
    char *target= argv[1];
    char *folder = argv[2];
    
    //usage kontrol
    if(argc !=3)
    {
        //Ekrana olası durumda hata basma
        fprintf(stderr, "Usage: %s which_word which_directname\n",argv[0]);
        exit(ONE);
    }
    listDirectory(target, folder);
    fileP=fopen("log.txt","a+");
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
    {
        return ZERO;
    }
    else 
    {
        return S_ISDIR(statbuf.st_mode);
    }
}

int listDirectory(const char* word, const char* directname)
{
    struct dirent *direntPtr;
    DIR *dirp;
    pid_t childPid;
    char newPathName[PATH_MAX];
    int checkDirectory =0;
    int sizeOfDirname =0;
    int status=0;
    int numOfTotalString =0;
    char fileName[PATH_MAX];
   
    
    if((dirp = opendir(directname)) == NULL)
    {
        perror("Failed to open directory");
        return 1;
    }
    
    while((direntPtr = readdir(dirp)) != NULL)
    {

        strcpy(newPathName, directname);
        char *str = direntPtr->d_name;
        sizeOfDirname = strlen(str);
        
        /*directory kontrolu yapilir, eger  "." , ".." veya "~" 
         ile biten directory ise gozardı edilir  */
        if(strcmp(direntPtr->d_name,".")!= ZERO &&
           strcmp(direntPtr->d_name,"..")!=ZERO &&
           direntPtr->d_name[sizeOfDirname-1]!='~' )
        {
            strcpy(newPathName, directname);
            strcat(newPathName,"/");
            strcat(newPathName, direntPtr->d_name);
        }
        checkDirectory=isDirectory(newPathName);
        /*Eger yeni pathimiz directory ise child process olusturularak recursive 
          cagrı yapılır ve directory icinde tarama yapılır.*/
        if(strcmp(direntPtr->d_name,".")!=ZERO &&
           strcmp(direntPtr->d_name,"..")!=ZERO &&
           direntPtr->d_name[sizeOfDirname-1]!='~' &&
           checkDirectory!=ZERO)
        {
            childPid= fork();  //yeni process oluşturma
            if(childPid < ZERO)  /*eger process olusmaz ise*/
            {
                printf("Fork failed.\n");
                exit(FALSE);
            }
            else if(childPid == ZERO) /* child process*/
            {
                listDirectory( word,newPathName);
                exit(ZERO);
            }
            else // anne process cocukların olmesini bekler
            {
                wait(&status);
            }
        }
       /* Eger pathimiz file ise cocuk process ile fonksiyonumuzu cagırarak 
          bulmak istedigimiz stringi ararız.  */
        else if(checkDirectory ==ZERO && strcmp(direntPtr->d_name,".")!=ZERO &&
                strcmp(direntPtr->d_name,"..")!= ZERO &&
                direntPtr->d_name[sizeOfDirname-1]!='~')
        {

            childPid = fork();
            if(childPid < ZERO)
            {
                printf("Fork failed.\n");
                exit(FALSE);
            }
            else if(childPid == ZERO)
            {
                strcpy(fileName, direntPtr->d_name);
                searchWordInFile(word, newPathName, fileName);
               
                exit(ZERO);
            }
            else
            {
                wait(&status);
            }

        }
    }
    closedir(dirp);
    
}

int searchWordInFile(const char *word, const char *folderName, const char *filename)
{

    char temp;
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
    FILE *fptr; // file pointer
    
    
    fptr= fopen("log.txt","a");
    
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
                    
                    fprintf(fptr, "%s: [%d,%d] %s first character is found.\n",
                            filename,lineCount, columnNum, word);
                    ++frequencyOfStr;
                    j=0;
                    break;
                }
            
            }
            j=0;
        }
        ++k;
    }

    fclose(fptr);
    
    free(line);  //alinan yeri geri verme
    close(iPath);  //dosya kapama
    return 0;

}
