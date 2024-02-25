#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"

bool active = false;
bool debug = false;
bool in_background = false;

void load_page(int recentsize, int finalsize, QueueNode* node);
int memFull();
void mem_set_line(char* filename, char* line, int index);
int getIndex(char* line);



void load_page(int recentsize, int finalsize, QueueNode* node) {

    char* filename;
    filename = node->pcb->filename;

    FILE* file = fopen(filename, "r");

    char* line;
    int lines_alr_read = recentsize * 3;
    int lines_toread = (finalsize - recentsize) *3;

    int linesread = 0;
    int count = 0;
    char ch;
    int index;

    while (!feof(file)) {
        
        line = calloc(1, 101);
        if (fgets(line, 101, file) != NULL && count < 3) {
            if (exists(line) != -1) {
                linesread++;
            }

            if (linesread >= lines_alr_read && exists(line) == -1) {
                if (memFull() == -1) {
                    printf("Page fault! Victim page contents:\n");
                    printf("result = %i", memFull());
                    for (int i = 0; i < 3; i++) {
                        printf("%s", mem_get_value_at_line(i));
                    }
                    printf("End of victim page contents.\n");
                    mem_free_lines_between(0,2);
                }
                //printf("%i\n", memFull());
                mem_set_line(filename, line, memFull());
                count++;
                linesread++;
                index = getIndex(line);
            }
            free(line);
        }
    }
    node->pcb->temp_size++;
    node->pcb->end = node->pcb->end + count;

    index = getIndex(line) - count + 1;

    
    
    fclose(file);

}

int process_initialize(char *filename){
    FILE* fp;
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    
    fp = fopen(filename, "rt");

    if(fp == NULL){
		return FILE_DOES_NOT_EXIST;
    }

    int error_code = load_file(fp, start, end, filename);
   
    int pageend = *end;

    FILE* file;
    file = fopen(filename, "r");

    int lines = 0;
    char ch;
    while(!feof(file)) {
        ch = fgetc(file);
        if(ch == '\n') {
            lines++;
        }
    }
    if (ch != '\n' && lines != 0) lines++;
    fclose(file);

    int newend = *start + (lines-1) ;
 
    //printf("\nFilename = %s and start = %i and end = %i\n", filename, *start, *end);
    if(error_code != 0){
        fclose(fp);
        return FILE_ERROR;
    }

    PCB* newPCB = makePCB(*start,*end, newend, filename);
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    ready_queue_add_to_tail(node);   
    
    return 0;
        
}

int shell_process_initialize(){
    //Note that "You can assume that the # option will only be used in batch mode."
    //So we know that the input is a file, we can directly load the file into ram
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    int error_code = 0;
    error_code = load_file(stdin, start, end, "_SHELL");
    if(error_code != 0){
        return error_code;
    }
    PCB* newPCB = makePCB(*start,*end, 0, "shell");
    newPCB->priority = true;
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_head(node);

    freopen("/dev/tty", "r", stdin);
    return 0;
}


bool execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    int end;

    
    for(int i=0; i<quanta ; i++){
        line = mem_get_value_at_line(pcb->PC++);
        //printf("filename = %s, line got : %s and PC = %i\n", pcb->filename, line, pcb->PC);

        in_background = true;

        if(pcb->priority) {
            pcb->priority = false;
        }

        if (node->pcb->end == node->pcb->PC-1) {
            if (pcb->temp_size < pcb->full_size) {
                load_page(pcb->temp_size, pcb->full_size, node);

                //printf("End = %i, start = %i, PC = %i\n", node->pcb->end, node->pcb->start, node->pcb->PC);
                //printShellMemory();

                ready_queue_add_to_tail(node);
                parseInput(line); 
                in_background = false;
                return false;

            }
        }


        if(pcb->PC>pcb->end){
            parseInput(line);

            if (pcb->temp_size < pcb->full_size) {
                load_page(pcb->temp_size, pcb->full_size, node);
                //printf("End111 = %i, start = %i, PC = %i\n", node->pcb->end, node->pcb->start, node->pcb->PC);
                
                ready_queue_add_to_tail(node); 
                //printShellMemory();
                return false;
            }
            //printf("newframe : filename = %s, end : %i, pc : %i, start : %i,  : %i, pcbptsize : %i\n", pcb->filename, pcb->end, pcb->PC, pcb->start, pcb->pt.size);
            terminate_process(node);
            
            in_background = false;
            return true;
    
        } 


        //printf("filename = %s, end : %i, pc : %i, start : %i, pcbptsize : %i\n", pcb->filename, pcb->end, pcb->PC, pcb->start, pcb->pt.size);
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }   
    }
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg){
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        if(is_ready_empty()){
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        if(!execute_process(cur, quanta)) {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char* policy){ //, bool mt){
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return SCHEDULING_ERROR;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(strcmp("FCFS",policy)==0){
        scheduler_FCFS();
    }else if(strcmp("SJF",policy)==0){
        scheduler_SJF();
    }else if(strcmp("RR",policy)==0){
        arg[0] = 2;
        scheduler_RR((void *) arg);
    }else if(strcmp("AGING",policy)==0){
        scheduler_AGING();
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        scheduler_RR((void *) arg);
    }
    return 0;
}
