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
   


    PAGE page_table[100];

    int newstart = start;
    int newend = end +1 ;

    if (newend % 3 != 0) {
        while (newend % 3 != 0){
            newend++;
        }
    }

    int i = 0;
    while(newend != newstart) {

        if (start == 0 && newstart == start) {
            page_table[i].page_id = newPCB->pid;
            page_table[i].frame_number = 0;
            //printf("End = %i and start = %i and frame# = %i and pID = %i\n", start+3, start, page_table[i].frame_number, page_table[i].page_id);
            newstart+=3;
            i++;
        }
        else if (newstart != newend) {
            page_table[i].page_id = newPCB->pid;
            page_table[i].frame_number = newstart/3;
            //printf("End = %i and start = %i and frame# = %i and pID = %i\n", newstart+3, newstart, page_table[i].frame_number, page_table[i].page_id);
            i++;
            newstart+=3;
        } else if (newend == newstart) {
            page_table[i].page_id = newPCB->pid;
            page_table[i].frame_number = newstart/3;
            //printf("End = %i and start = %i and frame# = %i and pID = %i\n", newstart+3, newstart, page_table[i].frame_number, page_table[i].page_id);
            i++;
            newstart+=3;
        }
        
    }

    return newPCB;
}