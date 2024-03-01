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
 
 
 
void load_page(int recentsize, int finalsize, QueueNode* node, PAGE* lastused);
int memFullorNewStart();
void mem_set_line(char* filename, char* line, int index);
int getIndex(char* line);
 
 
 
 
void load_page(int recentsize, int finalsize, QueueNode* node, PAGE* lastused) {
 
    char* filename;
    filename = node->pcb->filename;
    FILE* file = fopen(filename, "r");
 
    char* line;
    int lines_alr_read = recentsize * 3;
    int lines_toread = 3;;
    int linesread = 0;
 
    int count = 0;
    int index;
    int freed = 0;
    int lineNo = 0; 
    
 
    fseek(file, 0, SEEK_SET);
 
    int removefrom = lastused->start;
    int removeto = lastused->end;
 
    while ((line = calloc(1, 101)) != NULL && fgets(line, 101, file) != NULL && count < 4) {
        lineNo++;
 
        if (lineNo > lines_alr_read && count < lines_toread) {
           //printf(" line = %s lineNo = %i, linesalread = %i and linestoread = %i and count == %i, index of new lines = %i and exists = %i and memfull or new start = %i\n", line, lineNo, lines_alr_read, lines_toread, count, index, exists(line), memFullorNewStart());
           if (memFullorNewStart() == -1 && freed == 0 && count == 0) {   // if memory full, we free
                //printShellMemory();
                //printf("line loaded before page fault : %s\n", line);
                printf("Page fault! Victim page contents:\n");
                for (int i = removefrom; i <= removeto; i++) {
                    printf("%s", mem_get_value_at_line(i));
                }
                printf("End of victim page contents.\n");
                mem_free_lines_between(removefrom, removeto);
                freed = 1;
                lastused->end = 0;
                lastused->executed = 0;
                lastused->start = 0;
                lastused->last_used = 0;
                //printf("remove from = %i, remove to = %i\n", removefrom, removeto);
           }
 
           if (freed == 1) {   // if freed, we reset used page 
                count++;
                linesread++;
                //printf("\n currently putting the line : %s", line);
                if (count > 1) {
                    mem_set_line(filename, line, removefrom++);
                } else 
                    mem_set_line(filename, line, removefrom);
                index = getIndex(line);
                //printf(" and index of tha line is %i\n", removefrom);
                free(line); 
 
            } else { // if not freed, we set memory to free page
                //printf("\n currently putting the line : %s", line);
                count++;
                linesread++;
                if (count > 1) {
                    mem_set_line(filename, line, index + 1);
                } else 
                    mem_set_line(filename, line, memFullorNewStart());
                index = getIndex(line);
                //printf(" and index of tha line is %i\n", removefrom);
                free(line); 
            } 
        } else {
            free(line); 
            if (count >= lines_toread) break; 
        } 
    }
 
    //printf("in loadpage : tempsize = %i and full size = %i\n", node->pcb->temp_size , node->pcb->full_size);
 
    if (freed == 1) {       // IF WE FREED, PAGE WILL TAKE PLACE OF VICTIM CONTENT
        int k = 0;
        while (node->pcb->pt[k]->loaded != 0) {
            k++;
        }
 
        node->pcb->pt[k]->end = removefrom;
        node->pcb->pt[k]->start = removefrom - count + 1 ;
        node->pcb->pt[k]->loaded = 1;
        node->pcb->temp_size++; 
        node->pcb->PC = removefrom - count + 1;
        //printf("k = %i, START = %i, END = %i and LOADED = %i\n\n\n", k, node->pcb->pt[k]->start, node->pcb->pt[k]->end, node->pcb->pt[k]->loaded);
        node->pcb->end = removefrom ;
        //ready_queue_add_to_tail(node);
 
    } else {        // IF NOT FREED, PAGE WILL TAKE PLACE OF INDEX AT WHICH IT WAS PLACED.
        
        int i = 0;
        while (node->pcb->pt[i]->loaded != 0) {
            i++;
        }
        
        node->pcb->pt[i]->end = index;
        node->pcb->pt[i]->start = index - count + 1;
        node->pcb->pt[i]->loaded = 1;
        node->pcb->temp_size++;
        //printf("k = %i, START = %i, END = %i and LOADED = %i\n", i, node->pcb->pt[i]->start, node->pcb->pt[i]->end, node->pcb->pt[i]->loaded);
        node->pcb->PC = index - count + 1;
        node->pcb->end = index;
        //ready_queue_add_to_tail(node);
 
    }
 
    //printShellMemory();
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
 
    //printf("\nFilename = %s and start = %i and end = %i but acc end = %i\n", filename, *start, *end, newend);
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
 
 
int execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    int end;
 
    int i;
    for(i = 0 ; i<quanta ; i++){
        line = mem_get_value_at_line(pcb->PC++);
 
       // printf("\nline = %s", line);
       //printf("PC = %i, and end of frame = %i and i = %i\n", pcb->PC, pcb->end, i);
 
        int index = getIndex(line);
        int framenumber;
 
        if (quanta == MAX_INT) {
           for (int i = 0; pcb->pt[i]->loaded == 1; i++) {
                if (pcb->pt[i]->start <= index && pcb->pt[i]->end >= index) {
                    framenumber = i;
                }
            }   
 
            if (pcb->pt[framenumber]->end == pcb->PC-1) {
                for (int k = 0; pcb->pt[k]->loaded == 1; k++) {
                    if (k == framenumber) {    
                        pcb->pt[k]->executed = 1;
                        //printf("before parse : for filename %s and pcbid %i, im currently in framenumber %i\n", pcb->filename, pcb->pid, i);
                    } else {
                        pcb->pt[k]->last_used++;
                    } 
                }
            }   
        
        } else {
            for (int z = 0; pcb->pt[z]->loaded == 1; z++) {
                if (pcb->pt[z]->start <= index && pcb->pt[z]->end >= index) {
                    framenumber = z;
                }
            }   
 
            if (pcb->pt[framenumber]->end == pcb->PC-1) {     // MAKES A PAGE "EXECUTED" IF WE REACH END OF PAGE
                for (int q = 0; pcb->pt[q]->loaded == 1; q++) {
                    if (q == framenumber) {    
                        pcb->pt[q]->executed = 1;
                        //printf("for filename %s and pcbid %i, im currently in framenumber %i\n", pcb->filename, pcb->pid, i);
                    } 
                }
            }  
        }
 
 
        in_background = true;
 
        if(pcb->priority) {
            pcb->priority = false;
        }
 
        if(pcb->PC > pcb->end){
 
           //printf("still here\n");
 
            parseInput(line);
             
 
            for (int j = 0; pcb->pt[j]->loaded == 1; j++) { //INCREMENTS EXECUTED PAGES EXCEPT THE ONE JUST USED.
                if (pcb->pt[j]->executed == 1 && framenumber != j) {
                    pcb->pt[j]->last_used++; 
                    //printf("for filename %s and pcbid %i, im currently in framenumber %i and lastused = %i\n",pcb->filename, pcb->pid, j, pcb->pt[j]->last_used);
                }
            }  
 
 
            int small = 0;
            if (strcmp(getvariable(5), "none") == 0) {
                small = 1;
            }
            
            /*if (i == 0 && pcb->full_size > pcb->temp_size) {
                return 4;
            }*/
 
          
            if ((node->next == NULL && pcb->temp_size < pcb->full_size) || (!small && node->next == NULL && pcb->temp_size == pcb->full_size)) {
                return 2;
            }
 
          
        
            in_background = false;    
            return 1;
        }
 
        parseInput(line);
        in_background = false;
    }
 
    //printf("next filename : %s\n", node->next->pcb->filename);
    return 0;
}
 
 
void *scheduler_FCFS(){
    QueueNode *cur;
    int stillrunning = 0;
    int done = -1;
    
 
    while(true){
        if(is_ready_empty() && stillrunning == 0){
            if(active) 
                continue;
            else break;
        }
        cur = ready_queue_pop_head();
        
        done = execute_process(cur, MAX_INT);
 
        if (done == 2) {
            if (cur->pcb->temp_size < cur->pcb->full_size) {   // TO SEE IF WE SHOULD LOAD ANOTHER FILE OR NOT
                int lastused = 0;
                PAGE* lastusedframe;
                for (int j = 0; cur->pcb->pt[j]->loaded == 1; j++) {  //LRU IMPLEMENTATION FOR FCFS
                    if (cur->pcb->pt[j]->executed == 1) {
                        if (cur->pcb->pt[j]->last_used > lastused) {
                            lastused = cur->pcb->pt[j]->last_used;
                            lastusedframe = cur->pcb->pt[j];
                        }
                    }
                }
                load_page(cur->pcb->temp_size, cur->pcb->full_size, cur, lastusedframe);
                ready_queue_add_to_tail(cur);
                stillrunning = 1;
            } else {
                stillrunning = 0;
            }
        }
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
    QueueNode* totalPCB[3];
    int done = -1;
    int stillrunning = 1;
    int skip = 0;
    int loaded = 0;
    
    for (int i = 0; i < 3; i++) {
        totalPCB[i] = malloc(sizeof(QueueNode));
        totalPCB[i]->next = NULL;
        totalPCB[i]->pcb = NULL;
    }
 
    while(stillrunning == 1){
 
        if(is_ready_empty()){
            if(active) 
                continue;
            else break;
        }
 
        if (skip == 0) {
            cur = ready_queue_pop_head();
 
            int isAlreadyTracked = 0;
            for (int i = 0; i < 3; i++) {
                if (totalPCB[i]->pcb != NULL && totalPCB[i]->pcb->pid == cur->pcb->pid) {
                    isAlreadyTracked = 1;
                    break; 
                }
            }
            if (isAlreadyTracked == 0) {
                for (int i = 0; i < 3; i++) {
                    if (totalPCB[i]->pcb == NULL) {
                        totalPCB[i] = cur;
                        break; 
                    }
                }
            } 
            
            for (int i = 0; i < 3; i++) {       // INCREMENTS LAST USED OF EACH PAGE THAT WAS ALR EXECUTED
                if (totalPCB[i] != NULL && totalPCB[i]->pcb != NULL) {
                   for (int j = 0; totalPCB[i]->pcb->pt[j]->loaded == 1; j++) {
                        if (totalPCB[i]->pcb->pt[j]->executed == 1) {
                           totalPCB[i]->pcb->pt[j]->last_used++; 
                            //printf("IN RR SCHEDULER : for filename %s and pcbid %i, im currently in framenumber %i and lastused = %i\n",totalPCB[i]->pcb->filename, totalPCB[i]->pcb->pid, j, totalPCB[i]->pcb->pt[j]->last_used);
                        }
                   }   
                }
            }
            
            //printf("will currently execute filename : %s\n", cur->pcb->filename);
            done = execute_process(cur, quanta);
            //printf("done == %i\n\n", done);
        }
 
        //printf("done = %i\n", done);
        if(done == 0) {     // IF PROGRAM IS STILL NOT DONE BUT ONLY 2 LINES WERE EXECUTED
            ready_queue_add_to_tail(cur);
            skip = 0;
 
        } if (done == 1) {      // IF WE NEED CONTINUE RUNNING DESPITE HAVING REACHED THE END OF THE FILE
            if (cur->next == NULL)
                stillrunning = 0;
            if (cur->next != NULL)
                stillrunning = 1;
 
        } else if (done == 2) { // IF WE NEED TO LOAD ANOTHER PAGE
 
            loaded = 0;
            
            int c = 0;
            for (int i = 0; i < 3; i++) {
                if (totalPCB[i] != NULL && totalPCB[i]->pcb != NULL) {
                    //printf("PCB %i Filename: %s\n", i, totalPCB[i]->pcb->filename);  
                    c = c+1; 
                }
            }
           
            int k = -1;
 
            if (c == 1) {
                k = 0;
            } else if (c == 2) {
                if (cur->pcb->pid == totalPCB[0]->pcb->pid) {
                    k = 1;
                } else {
                    k = 0;
                }
            } else if (c == 3) {
               if (cur->pcb->pid == totalPCB[0]->pcb->pid) {
                    k = 1;
                } else if (cur->pcb->pid == totalPCB[1]->pcb->pid) {
                    k = 2;
                } else if (cur->pcb->pid == totalPCB[2]->pcb->pid) {
                    k = 0;
                } 
            }
 
            
            if (k >= 0 && totalPCB[k] != NULL) {
                for (int j = 0; j < c; j++) {
                    if (k == j) {
                        cur = totalPCB[j];
                       
                        /*if (done == 4 && skip == 0){
                            if (j != 0) j = j -1;
                            if (j == 0) j = 2;
                        }*/
                        //printf("k = %i, j= %i\n", k, j);
                        //printf("tempsize = %i and full size = %i\n", totalPCB[j]->pcb->temp_size , totalPCB[j]->pcb->full_size);
                        //printf("cur filename == %s\n", cur->pcb->filename);
                        if (present(cur->pcb->filename)) {
                            skip = 0;
                        }
                        else if (totalPCB[j]->pcb->temp_size < totalPCB[j]->pcb->full_size) {  
                            int lastused = 0;
                            PAGE* lastusedframe;
 
                            for (int i = 0; i < 3; i++) {   // CODE TO IDENTIDY LRU PAGE
                                if (totalPCB[i] != NULL && totalPCB[i]->pcb != NULL) {
                                    for (int z = 0; totalPCB[i]->pcb->pt[z]->loaded == 1; z++) {
                                        if (totalPCB[i]->pcb->pt[z]->executed == 1) {
                                            if (totalPCB[i]->pcb->pt[z]->last_used > lastused) {
                                                lastused = totalPCB[i]->pcb->pt[z]->last_used;
                                                lastusedframe = totalPCB[i]->pcb->pt[z];
                                                //printf("filename = %s and frame# = %i\n", totalPCB[i]->pcb->filename, j);
                                            }
                                        }
                                   }
                                }
                            } 
 
                            //printf("last used = == %i\n", lastused);
                            //printf("last used starts at : %i and end at %i\n", lastusedframe->start, lastusedframe->end);
                            load_page(totalPCB[j]->pcb->temp_size, totalPCB[j]->pcb->full_size, totalPCB[j], lastusedframe);  //LOADS A NEW PAGE
                            ready_queue_add_to_tail(totalPCB[j]);   // ADDS LOADED PAGE TO QUEUE
                            //printf("JUST FINISHED LOADING ANOTHER PAGE : %s\n", totalPCB[j]->pcb->filename);
                            loaded = 1;
                            if (c > 1){
                                skip = 1;
                            }
                        } else if (totalPCB[j]->pcb->temp_size == totalPCB[j]->pcb->full_size) {
                            if (j == 2) {
                                k = 0;
                            } else {
                                k++;
                                j++;
                            }
                        } // IF A FILE IS ALREADY DONE RUNNING, WE SKIP TO NEXT ONE
                    }
                }
            }
 
            
            int over = 0;
            for (int j = 0; j < c; j++) {   //CHECK TO SEE IF ALL PROGRAMS REACHED THEIR MAXIMUM SIZE/ ALL THEIR LINES READ
                //printf("tempsize = %i and full size = %i \n", totalPCB[j]->pcb->temp_size , totalPCB[j]->pcb->full_size);
                if (totalPCB[j]->pcb->temp_size == totalPCB[j]->pcb->full_size) {   
                    over++;
                }
            }
 
            //printf("over = %i, c = %i, loaded = %i\n\n\n", over, c, loaded);
            if (over == c && loaded == 0) {     // IF ALL PROGRAMS REACHED THEIR END GOAL/ WE'RE DONE
                if (cur->next) {                // AND WE ALSO DIDN'T JUST LOAD A FILE
                    //printf("asdsadsa");
                    stillrunning = 1;
                } else {
                    stillrunning = 0;
                }
            } else if (over == c && loaded == 1) { //ELSE IF A FILE HAS BEEN LOADED, AND ALL FILES REACHED MAX SIZE
                //printf("im here");               // WE STILL NEED TO EXECUTE IT BEFORE WE END.
                stillrunning = 1;    
                skip = 0;
            }
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