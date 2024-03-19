#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

#define SHELL_MEM_LENGTH (VARMEMSIZE + FRAMESIZE)

#ifndef FRAMESIZE
#define FRAMESIZE 300  
#endif

#ifndef VARMEMSIZE
#define VARMEMSIZE 700 
#endif


void mem_free_lines_between(int start, int end);
char * mem_get_value_at_line(int index);
int memFull();
void mem_set_line(char* filename, char* line, int index);
int getIndex(char* line);

struct memory_struct{
	char *var;
	char *value;
};

struct memory_struct shellmemory[VARMEMSIZE + FRAMESIZE];


// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions

void mem_init(){
	int fsize = FRAMESIZE;
	int varsize = VARMEMSIZE;

	int i;
	for (i=0; i<(VARMEMSIZE + FRAMESIZE); i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=FRAMESIZE; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=FRAMESIZE; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	return;
}

void frame_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<FRAMESIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<FRAMESIZE; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}
	return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i<(VARMEMSIZE + FRAMESIZE); i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}


void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < (FRAMESIZE + VARMEMSIZE); i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", VARMEMSIZE + FRAMESIZE, (VARMEMSIZE + FRAMESIZE)-count_empty, count_empty);
}


/*
 * Function:  addFileToMem 
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here
 *	pEnd: This function will store the last line of the loaded file 
 			in shell memory in here
 *  fileID: Input that need to provide when calling the function, 
 			stores the ID of the file
 * 
 * returns: error code, 21: no space left
 */
int load_file(FILE* fp, int* pStart, int* pEnd, char* filename) {
	char *line;
    size_t i;
    int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true;
	i = 0;
	size_t candidate;
	while(flag){
		flag = false;
		for (i; i < FRAMESIZE; i++){
			if(strcmp(shellmemory[i].var,"none") == 0 && strcmp(shellmemory[i+1].var,"none") == 0 && strcmp(shellmemory[i+2].var,"none") == 0){
				*pStart = (int)i;
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i;
		for(i; i < FRAMESIZE; i++){
			if(strcmp(shellmemory[i].var,"none") != 0){
				flag = true;
				break;
			}
		}
	}
	i = candidate;
	//shell memory is full
	int newstart = 0;
	if(hasSpaceLeft == 0){
		error_code = 21;
		return error_code;
	}
        
       
	   
	if (i % 3 != 0) {
		while(i % 3 != 0) {
			i++;
		}
		*pStart = (int)i;	// IF NEXT FREE SLOT IS NOT A MULTIPLE OF 3, MAKES IT A MULT OF 3
	} 

	int linesRead;

	int maxlines = 6;
	
    for (size_t j = i; (j < FRAMESIZE); j++){	// CAN READ MAX 6 LINES FROM EACH PROGRAM
        if (feof(fp) || linesRead == maxlines)
        {	
            *pEnd = (int)j-1;
            break;
        }else{	
			char *value;
			line = calloc(1, 101);
			if (fgets(line, 101, fp) == NULL) 
			{
				linesRead++;
				continue;
			}
			
			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			linesRead++;
			if (strncmp(line, "set ", 4) == 0) {		// IF LINE IS A SET COMMANDS, PUTS IT IN THE VARIABLE STORE
				char val[150], var[150];
				if (sscanf(line, "set %150s %150s", var, val) > 1)
					mem_set_value(var, val);
			}
			free(line);
        }

		if (j == FRAMESIZE - 1) {
			*pEnd = (int)j;
            break;	
		}
    }

	//no space left to load the entire file into shell memory
	if(!feof(fp) && linesRead < 6 && *pEnd == FRAMESIZE - 1){
		error_code = 21;
		//clean up the file in memory
		for(int j = 1; i <= SHELL_MEM_LENGTH; i ++){
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
    	}
		return error_code;
	}
	
	//printShellMemory();
    return error_code;
}



char * mem_get_value_at_line(int index){
	if(index<0 || index > (VARMEMSIZE + FRAMESIZE)) return NULL; 
	return shellmemory[index].value;
}

void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i< (VARMEMSIZE + FRAMESIZE); i++){
		if(shellmemory[i].var != NULL){
			shellmemory[i].var;
		}	
		if(shellmemory[i].value != NULL){
			shellmemory[i].value;
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}
char *getvariable(int index) {
	if(index<0 || index > (VARMEMSIZE + FRAMESIZE)) return NULL; 
	return shellmemory[index].var;
}

void *resetvariable(int index) {
	if (strcmp(shellmemory[index].value, "none") == 0){
		shellmemory[index].var = "none";
	}
}

int exists(char* variable) {		//CHECKS IF A LINE IS ALREADY IN MEMORY
	for (int i = 0; i < FRAMESIZE; i++) {
		if (strcmp(mem_get_value_at_line(i), variable) == 0) {
			return i;
		}
	}
	return -1;

}

int memFullorNewStart() {	//CHECKS IF FRAME STORE IS FULL OR WHERE THE FIRST FREE PAGE IS.
	int i;
	for (i = 0; i < FRAMESIZE; i+=3) {
		if (strcmp(mem_get_value_at_line(i), "none") == 0 && 
				strcmp(mem_get_value_at_line(i+1),"none") == 0 && 
				strcmp(mem_get_value_at_line(i+2), "none") == 0) {
			return i;
		}
	}
return -1;	// RETURNS -1 IF IS FULL
}

void mem_set_line(char* filename, char* line, int index) { // SETS A LINE AT A PARTICULAR INDEX
	for (int i = index; i < index+3; i++) {					// IF THERE IS A LINES THERE
		if (strcmp(mem_get_value_at_line(i), "none") == 0) {	// IT ADDS IT TO THE NEXT AVAILABLE SPOT
			shellmemory[i].value = strndup(line, strlen(line));
			shellmemory[i].var = filename;
			if (strncmp(line, "set ", 4) == 0) {
				char val[150], var[150];
				if (sscanf(line, "set %150s %150s", var, val) > 1)
					mem_set_value(var, val);
			}	
			break;
		}
	}
	
}

int getIndex(char* line) {	// GET INDEX OF A LINE IN THE FRAME STORE
	for (int i = 0; i < FRAMESIZE ; i++) {
		if (strcmp(mem_get_value_at_line(i), line) == 0) {
			return i;
		}
	}
}

int resetmem() { // RESETS THE FRAME STORE 
	for (int i = FRAMESIZE; i < SHELL_MEM_LENGTH; i++) {
		if (strcmp(mem_get_value_at_line(i), "none") != 0) {
			mem_set_value(getvariable(i), "none");
		}
		resetvariable(i);	
	}
    return 1;
}
