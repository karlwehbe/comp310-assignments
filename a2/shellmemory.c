#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

#define SHELL_MEM_LENGTH 1000
#define FRAMESTORE_SIZE 600

char *getvariable(int index);
void *resetvariable(int index);

struct memory_struct{
	char *var;
	char *value;
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH];
struct memory_struct frame_store[FRAMESTORE_SIZE];



void loadtoframe(int i, char* filename) {

	FILE* fp = fopen(filename, "r");

    ssize_t read;
	size_t len = 0;
	char* line;
	int linesRead = 0;

	if (i % 3 != 0) {
		while (i % 3 != 0) {
			i++;
		}
	}
	
	for (int j = i; j < FRAMESTORE_SIZE; j+=3) {
		linesRead = 0;
	
		while (j < FRAMESTORE_SIZE && linesRead < 3 && fgets(line, 20, fp) != NULL) {
			printf("%i", linesRead);
			
			line[strcspn(line, "\n")] = 0;
			printf("line is : %s \n", line);	
			frame_store[j + linesRead].var = filename; 
			frame_store[j + linesRead].value = line;
			linesRead++;
		}
	}
	fclose(fp);
}


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
	for (i=0; i<SHELL_MEM_LENGTH; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

void framestore_init() {
	for (int j = 0; j < FRAMESTORE_SIZE; j++) {
		frame_store[j].var = "none";
		frame_store[j].value = "none";	
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<SHELL_MEM_LENGTH; i++){	
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<SHELL_MEM_LENGTH; i++){		
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
	for (i=0; i<SHELL_MEM_LENGTH; i++){	
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
			printf("\nline %d in shell_mem: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in shell_mem total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH-count_empty, count_empty);
}

void printFrameMemory() {
	int count_empty = 0;
	for (int i = 0; i < FRAMESTORE_SIZE; i++){		
		if(strcmp(frame_store[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d in frame_store: key: %s\t\tvalue: %s\n", i, frame_store[i].var, frame_store[i].value);
		}
    }
	printf("\n\t%d lines in frame_store total, %d lines in use, %d lines free\n\n", FRAMESTORE_SIZE, FRAMESTORE_SIZE-count_empty, count_empty);	
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
	i=0;
	size_t candidate;

	while(flag && i < 300){
		flag = false;
		for (i; i < 300; i++){
			if(strcmp(shellmemory[i].var,"none") == 0){
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
	if(hasSpaceLeft == 0){	
		error_code = 21;
		return error_code;
	}


    for (size_t j = i; j < 300; j++){
        if(feof(fp)) {
            *pEnd = (int)j-1;
            break;
        } else{
			line = calloc(1, SHELL_MEM_LENGTH);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL)
			{
				continue;
			}

			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			free(line);
		
        }
    }

	loadtoframe(i, filename);

	//no space left to load the entire file into shell memory
	if(!feof(fp)){
		error_code = 21;
		//clean up the file in memory
		for(int j = 1; i <= SHELL_MEM_LENGTH; i ++){
			frame_store[j].var = "none";
			frame_store[j].value = "none";
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