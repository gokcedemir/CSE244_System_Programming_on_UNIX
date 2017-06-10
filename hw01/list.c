/*============================================================================*/
/* Bu program parametre olarak aldığı stringi, parametre olarak aldigi        */
/* dosyanin icinde arar. Aranan stringin ilk harfinin bulundugu satiri,		  */
/* sutunu ve verilen dosyada toplam kac string'den oldugunu print eder.       */
/*                                                                            */
/* File:   list.c                                                             */
/* Author: Gökçe Demir - 141044067                                            */
/*  HW1 CSE244 System Programming                                             */
/* Created on February 27, 2017, 5:51 PM                                      */
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>  // for stderr

/* MACROS   */ 
#define READ_FLAGS O_RDONLY  //file macro
#define ZERO 0
#define ONE 1

/*============================================================================*/
/*  Dosya icinde bir stringin kac defa gectigini bulur. Parametre olarak      */
/*	string ve dosya adını .						                              */
/*	Return Degeri: Integer return eder, dogru ise return 0, basarısız olur    */
/*	ise sifirdan farkli bir deger return eder.	                              */
/*============================================================================*/
int searchWordInFile(const char *word, const char *filename);

int main(int argc, char** argv) {

    //usage kullanımı 
    if(argc !=3)
    {
		//Ekrana olası durumda hata basma
        fprintf(stderr, "Usage: %s which_word which_file\n",argv[0]);
        exit(1);
    }
	//function cagrilir
    searchWordInFile(argv[1], argv[2]);

    return 0;
}

int searchWordInFile(const char *word, const char *filename) {

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
    
    // check file 
    if((iPath = open(filename, READ_FLAGS))== -1)
    {
        perror("Failed to open input file");
        return 1;
    }
    
    for (i = 0; (x = read(iPath,readByte,1))!= 0; i++);
    sizeOfArray=i;  // dosya boyutu kadar array ayıracagiz
    
    line = (char*) (calloc(sizeOfArray, sizeof (char)));
    
    iPath = open(filename,READ_FLAGS);
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
                    fprintf(stdout, "[%d,%d] konumunda ilk karekter bulundu.\n",
													 lineCount, columnNum);
                    ++frequencyOfStr;
                    j=0;
                    break;
                }
            
            }
            j=0;
        }

        ++k;
     

    }

    printf("\n%d kere %s bulundu.\n", frequencyOfStr, word);
    
	
    free(line);  //alinan yeri geri verme
    close(iPath);  //dosya kapama
    return 0;

}
