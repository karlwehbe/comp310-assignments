#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */

typedef struct PAGE PAGE;
struct PAGE {
    int start;
    int end;
    int loaded;
    int executed;
    int last_used;
};
typedef struct
{
    bool priority;
    int pid;
    int PC;
    int start;
    int end;
    int job_length_score; 
    PAGE* pt[30]; 
    int endof_file;
    int temp_size;
    int full_size;
    char* filename;
}PCB;

int generatePID();
PCB * makePCB(int start, int end, int accend, char* filename);
#endif