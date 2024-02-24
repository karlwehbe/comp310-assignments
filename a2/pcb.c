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
PCB* makePCB(int start, int end){
    
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start  = start;
    newPCB->end = end;
    newPCB->job_length_score = 1+end-start;
    newPCB->priority = false;
    newPCB->pt = genpt(start, end, pid_counter);

    return newPCB;

}

PAGE genpt(int start, int end, int pid) {
    PAGE pagetable[((end-start)/3)+1] ;
    int newstart = start;
    int i = 0;
    
    while(end >= newstart) {

        if ((newstart+2) < end) {
            pagetable[i].frame_number = newstart/3;
            pagetable[i].pageid = pid;
            pagetable[i].start = newstart;
            pagetable[i].end = newstart+2;
            pagetable[i].size = ((end-start)/3) + 1;
            //printf("PID of this frame is %i __________ specifics include : INDEX = %i, FRAME #%i, and STARTS at line %i and ENDS at line %i and IS : %d\n", pagetable[i].pageid, i, pagetable[i].frame_number, pagetable[i].start, pagetable[i].end, pagetable[i].loaded);
            i++;
            newstart+=3;
        } else {
            pagetable[i].frame_number = newstart/3;
            pagetable[i].pageid = pid;
            pagetable[i].start = newstart;
            pagetable[i].end = end;
            pagetable[i].size = ((end-start)/3) + 1;
            //printf("PID of this frame is %i __________ specifics include : INDEX = %i, FRAME #%i, and STARTS at line %i and ENDS at line %i and IS: %d \n", pagetable[i].pageid, i, pagetable[i].frame_number, pagetable[i].start, pagetable[i].end, pagetable[i].loaded);
            i++;
            newstart+=3;

        }
    } 

    return *pagetable;
}

