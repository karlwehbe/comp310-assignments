#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

#define SHELL_MEM_LENGTH 1000

void mem_free_lines_between(int start, int end);
char * mem_get_value_at_line(int index);

struct memory_struct{
	char *var;
	char *value;
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH];

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
	int i;
	for (i=0; i<1000; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=300; i<1000; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=300; i<1000; i++){
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
	for (i=0; i<300; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<300; i++){
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
	for (i=0; i<1000; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}


void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH-count_empty, count_empty);
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
		for (i; i < 300; i++){
			if(strcmp(shellmemory[i].var,"none") == 0 && strcmp(shellmemory[i+1].var,"none") == 0 && strcmp(shellmemory[i+2].var,"none") == 0){
				*pStart = (int)i;
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i;
		for(i; i < 300; i++){
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
        printf("Page fault! Victim page contents:\n");
        for (int i = newstart; i < 3; i++) {
            printf(mem_get_value_at_line(i));
        }
        mem_free_lines_between(i, i+2);
        printf("End of victim page contents.\n");
		i = newstart;
    }
       
	   
	if (i % 3 != 0) {
		while(i % 3 != 0) {
			i++;
		}
		*pStart = (int)i;
	} 

	int linesRead;

	int maxlines = 6;
	
    for (size_t j = i; (j < 300); j++){
        if (feof(fp) || linesRead == maxlines)
        {	
            *pEnd = (int)j-1;
            break;
        }else{	
			char *value;
			line = calloc(1, SHELL_MEM_LENGTH);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL) 
			
			{
				linesRead++;
				continue;
			}
			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			linesRead++;
			if (strncmp(line, "set ", 4) == 0) {
				char val[100], var[100];
				if (sscanf(line, "set %99s %99s", var, val) > 1)
					mem_set_value(var, val);
			}
			free(line);
			
        }
    }

	//no space left to load the entire file into shell memory
	if(!feof(fp) && linesRead < 6){
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
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}

void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
		if(shellmemory[i].var != NULL){
			free(shellmemory[i].var);
		}	
		if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}
char *getvariable(int index) {
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].var;
}

void *resetvariable(int index) {
	if (strcmp(shellmemory[index].value, "none") == 0){
		shellmemory[index].var = "none";
	}
}

int exists(char* variable) {
	for (int i = 0; i < 300; i++) {
		if (strcmp(mem_get_value_at_line(i), variable) == 0) {
			return i;
		}
	}
	return -1;

}

/*int loadpage(FILE* fp, int* pStart, int* pEnd,  char* filename) { 

	char *line;
    size_t i;
    int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true;
	i = 0;
	size_t candidate;
	
	
	if (i % 3 != 0) {
		while(i % 3 != 0) {
			i++;
		}
		*pStart = (int)i;
	} 

	int linesRead;

	int maxlines = 3;
	
    for (size_t j = i; (j < 300); j++){
        if (feof(fp) || linesRead == maxlines)
        {	
            *pEnd = (int)j-1;
            break;
        }else{	
			char *value;
			line = calloc(1, SHELL_MEM_LENGTH);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL) 
			
			{
				linesRead++;
				continue;
			}
			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			linesRead++;
			if (strncmp(line, "set ", 4) == 0) {
				char val[100], var[100];
				if (sscanf(line, "set %99s %99s", var, val) > 1)
					mem_set_value(var, val);
			}
			free(line);
			
        }
    }

    return error_code;

}
*/