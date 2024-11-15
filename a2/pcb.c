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
    newPCB->temp_size = ((end - start)/3) + 1;      // CURRENT SIZE OF THE FILE / 2 AT THE START SINCE WE LOAD 2 PAGES ONLY
    newPCB->full_size = (acc_end - start)/3 + 1;    // SIZE OF THE FILE'S PAGE TABLE WHEN ALL LINES ARE READ
    newPCB->filename = filename;

    int numofpages =((end - start)/3) + 1;
  
    int newstart = start;
    int i = 0;
    
    for (int i = 0; i < 30; i++) {      // INITIALIZES THE PAGES OF THE PAGE TABLE
        if (i < numofpages) {
            newPCB->pt[i] = malloc(sizeof(PAGE));

            newPCB->pt[i]->start = newstart;
            newPCB->pt[i]->end = newstart + 2;
            newPCB->pt[i]->loaded = 1;          // LOADED INDICATES THAT A PAGE IS CURRENTLY PRESENT AT THIS INDEX
            newPCB->pt[i]->executed = 0;
            newPCB->pt[i]->last_used = 0;

            newstart += 3; 
       
        } else {   
            newPCB->pt[i] = malloc(sizeof(PAGE));       // INITIALIZES THE REST OF THE UNUSED PAGE TABLES

            newPCB->pt[i]->start = 0;
            newPCB->pt[i]->end = 0;
            newPCB->pt[i]->loaded = 0; 
            newPCB->pt[i]->executed = 0;
            newPCB->pt[i]->last_used = 0;
        }
    }

    return newPCB;
}

