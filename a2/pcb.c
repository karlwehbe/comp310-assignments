#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;


int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int start, int end, int acc_end, char* filename) {

    PAGE pagetable[((acc_end-start)/3)+1] ;

    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start  = start;
    newPCB->end = end;
    newPCB->job_length_score = 1+end-start;
    newPCB->priority = false;
    newPCB->endof_file = acc_end;
    newPCB->temp_size = ((end - start)/3) + 1;
    newPCB->full_size = (acc_end - start)/3 + 1;
    newPCB->filename = filename;


    int newstart = start;
    int i = 0;
    
    while(end >= newstart) {

        if ((newstart+2) < end) {
            pagetable[i].frame_number = i;
            pagetable[i].pageid = newPCB->pid;
            pagetable[i].start = newstart;
            pagetable[i].end = newstart+2;
            pagetable[i].size = ((end-start)/3) + 1;
            pagetable[i].loaded = true;
            //printf("PID of this frame is %i __________ specifics include : FRAME #/INDEX : %i, and STARTS at line %i and ENDS at line %i\n", pagetable[i].pageid, pagetable[i].frame_number, pagetable[i].start, pagetable[i].end);
            i++;
            newstart+=3;
        } else {
            pagetable[i].frame_number = i;
            pagetable[i].pageid = newPCB->pid;
            pagetable[i].start = newstart;
            pagetable[i].end = end;
            pagetable[i].size = ((end-start)/3) + 1;
            pagetable[i].loaded = false;
            //printf("PID of this frame is %i __________ specifics include : INDEX/FRAME # : %i, and STARTS at line %i and ENDS at line %i\n", pagetable[i].pageid, pagetable[i].frame_number, pagetable[i].start, pagetable[i].end);
            i++;
            newstart+=3;

        }
    }

    newPCB->pt = *pagetable;

    return newPCB;
}

