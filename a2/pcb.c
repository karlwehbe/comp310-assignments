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
    newPCB->page_table = generatePageTable(start, end, pid_counter);   
    newPCB->pt = genpt(start, end);

    return newPCB;

}

int genpt(int start, int end) {
    int pagetable[100];

    int newstart = start;

    while(end >= newstart) {
        pagetable[i] = newstart/3;
        //printf("Index = %i, frame# %i, and starts at line %i\n", i, pagetable[i], 3*pagetable[i]);
        i++;
        newstart+=3;
    } 

    return *pagetable;
}

PAGE generatePageTable(int start, int end, int pid) {

    PAGE page_table[100];

    int newstart = start;

    int i = 0 ;
    while(end >= newstart) {

        if (start == 0 && newstart == start) {
            page_table[i].pageid = pid;
            page_table[i].frame_number = 0;
            page_table[i].start = 0;
            page_table[i].end = start+2;
            //printf("Starts at = %i and ends at = %i and frame# = %i and pID = %i\n", page_table[i].start, page_table[i].end, page_table[i].frame_number, page_table[i].pageid);
            newstart+=3;
            i++;

        } else if (newstart <= end) {
            page_table[i].pageid = pid;
            page_table[i].frame_number = i;
            page_table[i].start = newstart;
            page_table[i].end = newstart+2 ;
            //printf("Starts at = %i and ends at = %i and frame# = %i and pID = %i\n", page_table[i].start, page_table[i].end, page_table[i].frame_number, page_table[i].pageid);
            i++;
            newstart+=3;
        }
    }
    
    return *page_table;   
}