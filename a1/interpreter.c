#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/stat.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 7;

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int echo(char* var);
int my_ls();
int my_mkdir(char* value);
int my_touch(char *value);
int my_cd(char* value);
int my_cat(char* value);

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;

	if ( args_size < 1 || args_size > MAX_ARGS_SIZE){
		return badcommand();
	}

	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help") == 0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit") == 0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set") == 0) {
		if (args_size < 3) return badcommand();	
		char* var = command_args[1];
        char* value = command_args[2];

        for (i = 3; i < args_size; i++) {
            strcat(value, " ");
            strcat(value, command_args[i]);
        }
        return set(var, value);
	
	} else if (strcmp(command_args[0], "print") == 0) {
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "run") == 0) {
		if (args_size != 2) return badcommand();
		return run(command_args[1]);
	
	} else if (strcmp(command_args[0], "echo") == 0) {
		if (args_size != 2) return badcommand();
		return echo(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1) return badcommand();
        return my_ls();

	} else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2) return badcommand();
        return my_mkdir(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2) return badcommand();
        return my_touch(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2) return badcommand();
        return my_cd(command_args[1]);

	} else if (strcmp(command_args[0], "my_cat") == 0) {
        if (args_size != 2) return badcommand();
        return my_cat(command_args[1]);

	} else return badcommand();
}

int help(){

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n \
echo 			Displays strings passed as arguments on the command line\n";

	printf("%s\n", help_string);
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	exit(0);
}


int set(char* var, char* value) {
	char *link = "=";
    char buffer[1000];
    strcpy(buffer, var);
    strcat(buffer, link);
    strcat(buffer, value);

    mem_set_value(var, value);

    return 0;
}

int print(char* var){
	printf("%s\n", mem_get_value(var)); 
	return 0;
}

int run(char* script){
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script,"rt");  // the program is in a file

	if(p == NULL){
		return badcommandFileDoesNotExist();
	}

	fgets(line,999,p);
	while(1){
		errCode = parseInput(line);	// which calls interpreter()
		memset(line, 0, sizeof(line));

		if(feof(p)){
			break;
		}
		fgets(line,999,p);
	}

    fclose(p);

	return errCode;
}

int echo(char* var) {
	if(var[0] != '$') { 
		printf("%s\n", var);
		return 0;
	}
	char s[101];
	strcpy(s, var);
	if (s[0] == '$') memmove(s, s+1, strlen(s));
	
	printf("%s\n", mem_get_value(s));

	return 0;
}


int my_ls(){
	system("ls");
	return 0;
}


int my_mkdir(char* value) {
	char command[110];
	
	strcpy(command, "mkdir ");
	strcat(command, value);
	
	system(command);

    return 0;
}


int my_touch(char* value) {
	char command[110];
	
	strcpy(command, "touch ");
	strcat(command, value);
	
	system(command);

    return 0;
}


int my_cd(char* value) {
	int result = chdir(value);

	if (result = 0) {
		printf("Bad command : my_cd");
		return 1;
	}
    return 0;
}


int my_cat(char* value) {
	FILE* file = fopen(value, "r");

    if (file != NULL) {
        char buffer[1000];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            printf("%s", buffer);
        }
		printf("\n");

        fclose(file);
        return 0;
		
    } else {
        printf("Bad command: my_cat");
        return 1;
    }
}