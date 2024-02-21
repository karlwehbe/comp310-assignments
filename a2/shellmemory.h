#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pStart, int* pEnd, char* fileID);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
char *getvariable(int index);
void *resetvariable(int index);
void frame_set_value(char *var_in, char *value_in);
#endif