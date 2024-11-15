#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "interpreter.h"
#include "shellmemory.h"
#include "pcb.h"
#include "kernel.h"
#include "shell.h"

#ifndef FRAMESIZE
#define FRAMESIZE 300  
#endif

#ifndef VARMEMSIZE
#define VARMEMSIZE 700 
#endif

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

int main(int argc, char *argv[]) {
    //backing store goes here
    
    chdir("/code");
    if (chdir("backingstore") == 0) {   //CHECKS IF BACKINGSTORE IS PRESENT
        chdir("..");                    // IF PRESENT, WE GO BACK TO /CODE
        system("rm -r backingstore"); 
    }
    system("mkdir backingstore");   //MAKES THE BACKINGSTORE IN BOTH CASES  
  
    int fsize = FRAMESIZE;      // FRAMESIZE INPUT VALUE IN THE MAKEFILE
    int memsize = VARMEMSIZE;   // VARMEMSIZE INPUT VALUE IN THE MAKEFILE
	printf("%s\n", "Shell v2.0\n");
    printf("Frame Store Size = %i; Variable Store Size = %i\n", fsize, memsize);	
    
    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
	int errorCode = 0;					// zero means no error, default

	//init user input
	for (int i=0; i<MAX_USER_INPUT; i++)
		userInput[i] = '\0';
	
	//init shell memory
	mem_init();
   
	while(1) {						
        if (isatty(fileno(stdin))) printf("%c ",prompt);

		char *str = fgets(userInput, MAX_USER_INPUT-1, stdin);
        if (feof(stdin)){
            freopen("/dev/tty", "r", stdin);
        }

		if(strlen(userInput) > 0) {
            errorCode = parseInput(userInput);
            if (errorCode == -1) exit(99);	// ignore all other errors
            memset(userInput, 0, sizeof(userInput));
		}
	}

	return 0;

}

int parseInput(char *ui) {
    char tmp[200];
    char *words[100];
	memset(words, 0, sizeof(words));
	
    int a = 0;
    int b;                            
    int w=0; // wordID    
    int errorCode;
    for(a=0; ui[a]==' ' && a<1000; a++);        // skip white spaces
    
    while(a<1000 && a<strlen(ui) && ui[a] != '\n' && ui[a] != '\0') {
        while(ui[a]==' ') a++;
        if(ui[a] == '\0') break;
        for(b=0; ui[a]!=';' && ui[a]!='\0' && ui[a]!='\n' && ui[a]!=' ' && a<1000; a++, b++) tmp[b] = ui[a];
        tmp[b] = '\0';
        if(strlen(tmp)==0) continue;
        words[w] = strdup(tmp);
        if(ui[a]==';'){
            w++;
            errorCode = interpreter(words, w);
            if(errorCode == -1) return errorCode;
            a++;
            w = 0;
            for(; ui[a]==' ' && a<1000; a++);        // skip white spaces
            continue;
        }
        //printf("words bef= %s\n", words[w]);
        w++;
        a++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}

