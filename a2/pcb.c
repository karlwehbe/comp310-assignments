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

    int numofpages =((end - start)/3) + 1;
  
    int newstart = start;
    int i = 0;
    
    for (int i = 0; i < 10; i++) {
        if (i < numofpages) {
            newPCB->pt[i] = malloc(sizeof(PAGE));

            newPCB->pt[i]->start = newstart;
            newPCB->pt[i]->end = (newstart + 2 <= end) ? newstart + 2 : end;
            newPCB->pt[i]->loaded = 1; 
            newPCB->pt[i]->executed = 0;
            newPCB->pt[i]->last_used = 0;

            newstart += 3; 
            //printf("START = %i, END = %i, LOADED = %i\n", newPCB->pt[i]->start, newPCB->pt[i]->end, newPCB->pt[i]->loaded);
        } else {   
            newPCB->pt[i] = malloc(sizeof(PAGE));

            newPCB->pt[i]->start = newstart;
            newPCB->pt[i]->end = (newstart + 2 <= end) ? newstart + 2 : end;
            newPCB->pt[i]->loaded = 0; 
            newPCB->pt[i]->executed = 0;
            newPCB->pt[i]->last_used = 0;

            newstart += 3; // Move to the next set of lines
            //printf("START = %i, END = %i, LOADED = %i\n",  newPCB->pt[i]->start, newPCB->pt[i]->end, newPCB->pt[i]->loaded);
        }
    }


    return newPCB;
}

