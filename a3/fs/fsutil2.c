#include "fsutil2.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Karl WEHBE - 261085350


int copy_in(char *fname) {
    FILE *source = fopen(fname, "r");
    if (!source) return 1;

    fseek(source, 0, SEEK_END);
    int fileSize = ftell(source);
    rewind(source);

    int newSize = 0;
    int sectorsavailable = fsutil_freespace() - 1;
    
    int sectors_towrite =  0;
    if (fileSize % 512 != 0) {
        sectors_towrite = (fileSize/512) + 1;
    } else {
        sectors_towrite = (fileSize/512);
    }

    int allocated_indirect = 0;
    if (sectorsavailable > 123 && sectors_towrite > 123) {                  //checks if we will need to allocate indirect blocks and how many
        if (sectorsavailable > 123+129 && sectors_towrite > 123+129) {          
            allocated_indirect = 17;
        } else {
           allocated_indirect = 2;
        }
    } else { allocated_indirect = 0; }

    if (sectorsavailable - allocated_indirect < sectors_towrite) {      //if file is bigger than available space (counting also the indirect and doubly indirect blocks)
        newSize = (sectorsavailable - allocated_indirect) * 512 - 1;    // we change the size/number of bytes that will be read from the file to the maximum possible.
    } else {
        newSize = fileSize;                                             // if not, we keep the same size
    }       

    int res = fsutil_create(fname, newSize);
    if (res != 1) {
        fclose(source);
        return 2;
    }

    if (newSize < fileSize) {
        printf("Warning: could only write %d out of %i bytes (reached end of file)\n", newSize + 1, fileSize);
    }

    char buffer[newSize];
    for (int i = 0; i < newSize; i++) {         
        buffer[i] = fgetc(source);              //reads file one char at a time
    }
    
    fsutil_write(fname, buffer, newSize + 1);       
    fclose(source);
    return 0;
}

int copy_out(char *fname) {
    struct file *f = filesys_open(fname);
    int size = fsutil_size(fname);
    char* buffer =  malloc((size+1) * sizeof(char));
    memset(buffer, 0, size + 1);

    int offset = 0;
    if (f != NULL) offset = file_tell(f);

    fsutil_seek(fname, 0);
    fsutil_read(fname, buffer, size);

    FILE* file = fopen(fname, "w");
    if (file == NULL) {
        fsutil_seek(fname, offset);    // take pointer back to original place.
        return 1;
    }

    fputs(buffer, file);
    fclose(file);
    free(buffer);
    
    fsutil_seek(fname, offset);    // take pointer back to original place.
    return 0;
}

void find_file(char *pattern) {
    struct dir *dir;
    char name[NAME_MAX + 1];

    dir = dir_open_root();
    if (dir == NULL)
        return ;

    while (dir_readdir(dir, name)) {            //goes threw all files in the directory 
        int size = fsutil_size(name);
        char* buffer =  malloc((size+1) * sizeof(char));
        memset(buffer, 0, size + 1);
        fsutil_read(name, buffer, size);

        fsutil_seek(name, 0);
        fsutil_read(name, buffer, size);

        if (strstr(buffer, pattern)) {      //checks if any files contains the pattern and if it does, prints the filename .
            printf("%s\n", name);
        }
    }
    dir_close(dir);
    return;
}


void fragmentation_degree() {
    struct dir *dir;
    char name[NAME_MAX + 1];

    dir = dir_open_root();
    if (dir == NULL)
        return ;

    int n_fragmented = 0;
    int n_fragmentable = 0;

    while (dir_readdir(dir, name)) {        //goes threw all files in directory.
        struct file* f = filesys_open(name);
        struct inode* node = file_get_inode(f);
        
        int sectors_to_read = 0;
        if (fsutil_size(name) % 512 != 0) {
            sectors_to_read = (fsutil_size(name)/512) + 1;
        } else {
            sectors_to_read = (fsutil_size(name)/512);
        }
    
        if (sectors_to_read > 1) {      //if the file occupies more than 1 sector.
            n_fragmentable++;            
            block_sector_t *blocks = get_inode_data_sectors(node);
            for (int i = 0; i < sectors_to_read; i++) {
                int place = 0;
                if (i == 0) {
                place = blocks[0];
                } else {
                place = blocks[i-1];
                }

                if (blocks[i] - place > 3 && blocks[i] != 0) {      //checks if the difference between 2 contiguous sectors is > 3
                    n_fragmented++;
                    break;
                }
            }
        }
    }
    dir_close(dir);

    float degree = (float)n_fragmented / n_fragmentable;
    printf("Num fragmentable files: %i\n", n_fragmentable);
    printf("Num fragmented files: %i\n", n_fragmented);
    printf("Fragmentation pct: %f\n", degree);
}



int defragment() {
    struct dir *dir;
    char name[NAME_MAX + 1];
   
    dir = dir_open_root();
    if (dir == NULL) return 1;

    int n_files = 0;
    while (dir_readdir(dir, name)) {
          n_files++;        //checks the number of files in the directory to know how much space to allocate for files array.
    }
    dir_close(dir);

    if (n_files == 0) {
        return 0;
    }

    struct newfiles* files = malloc(n_files * sizeof(struct newfiles));    //creates an array that will store information about each file
    if (files == NULL) {
        return -1;
    }

    struct dir *dir2 = dir_open_root();
    char fname[NAME_MAX + 1];
    int i = 0;
    while (dir_readdir(dir2, fname) && i < n_files) {    // for each file in the directory V
        int size = fsutil_size(fname);              
        files[i].content = malloc(size + 4);            
        files[i].name = strdup(fname);          // puts filename into array
        files[i].size = size;                   // puts size into array

        struct file* f = filesys_open(fname);
        file_seek(f, 0);
        file_read(f, files[i].content, size);   // puts the content of the file into array
        i++;
    }
    dir_close(dir2);

    for (int i = 0; i < n_files; i++) {
        fsutil_rm(files[i].name);           //removes all old files
    }
  
    for (int i = 0; i < n_files; i++) {
        fsutil_create(files[i].name, files[i].size);
        fsutil_write(files[i].name, files[i].content, files[i].size);     //adds all files one at a time so that their datablocks can be contiguous.
        free(files[i].name);
        free(files[i].content);
    }
    free(files);
    return 0;
}



void recover(int flag) {
  if (flag == 0) {            // recover deleted inodes
   
    int freesectors = 0;
    for (int i = 0; i < bitmap_size(free_map); i++) {

        if (!bitmap_test(free_map, i)) {    // If the i-th bit is 0 (free sector), gives 1 if removed/empty
            struct inode *node = inode_open(i);     //only gives an inode if its the sector of the inode, if represnts the data, it will not give back an inode.
            struct inode_disk id = node->data;
            struct file* f = file_open(node);

            if (id.length > 0 && !id.is_dir && node->sector > 0) {
                node->removed = 0;
                bitmap_set(free_map, i, 1);     //resets the corresponding inode sector bit to 1;
                freesectors++;

                int sectors_to_read = 0;
                if (id.length % 512 != 0) {
                    sectors_to_read = i + (id.length/512 + 1);
                } else {
                    sectors_to_read = i + (id.length/512);
                }

                for (int j = i; j < sectors_to_read; j++) {
                    bitmap_set(free_map, j, 1);         //resets the bits corresponding to the data blocks of the file to 1.
                    freesectors++;
                }

                char newname[15]; 
                sprintf(newname, "recovered0-%u", node->sector); 
                add_to_file_table(f, newname);
                
                struct dir* root = dir_open_root();
                dir_add(root, newname, i, false);
            }
        }
    }


  } else if (flag == 1) { // recover all non-empty sectors
  
        for (int i = 4; i < bitmap_size(free_map); i++) {   //to iterates through the bitmap

            if (bitmap_test(free_map, i)) {    // If the i-th bit is 1, gives 1.
                char buffer[1024];
                char newname[18]; 
                sprintf(newname, "recovered1-%u.txt", i); 
                memset(buffer, 0, sizeof(buffer)); 
                buffer_cache_read(i, buffer);       //puts the content of the sector into the buffer
            
                bool isEmpty = true;      
                for (size_t j = 0; j < sizeof(buffer); j++) {
                    if (buffer[j] != 0) {           // checks that the buffer isn't empty.
                        isEmpty = false;       
                        break;
                    }
                }

                if (!isEmpty) {
                    FILE* file = fopen(newname, "w");
                    fputs(buffer, file);        //saves the content of the sector into a file on the real filesystem
                    fclose(file);
                }
            }
        } 
    
  } else if (flag == 2) { // data past end of file.
        struct dir *dir;
        char name[NAME_MAX + 1];

        dir = dir_open_root();
        if (dir == NULL)
            return ;
        
        while (dir_readdir(dir, name)) {
            struct file* f = filesys_open(name);
            struct inode* node = file_get_inode(f);
            
            int sectors_to_read = 0;
            if (fsutil_size(name) % 512 != 0) {     //checks the number of sectors that a file occupies
                sectors_to_read = (fsutil_size(name)/512) + 1;
            } else {
                sectors_to_read = (fsutil_size(name)/512);
            }
      
            block_sector_t *blocks = get_inode_data_sectors(node); 
            int last_block = 0;
            for (int i = 0; i < sectors_to_read; i++) {
                if (blocks[i] != 0) {
                    last_block = blocks[i];             // iterate to find the files last block.
                }
            }

            if (bitmap_test(free_map, last_block)) {        //checks that the sector is non-zero
                int remaining_bytes = 512 - (fsutil_size(name) % 512);  //checks how many bytes will need to be read.
                int file_sectorbytes = 512 - remaining_bytes;   // checks how many bytes in the sector belong to the file

                char buffer[512];
                memset(buffer, 0, sizeof(buffer)); 
                buffer_cache_read(last_block, buffer);      //reads content of that sector.
                memmove(buffer, buffer + file_sectorbytes, sizeof(buffer) - file_sectorbytes);    // removes all the contents in the sector belonging to the file and keeps what doesn't belong to it.
                
                bool isEmpty = true;
                for (size_t j = 0; j < sizeof(buffer); j++) {
                    if (buffer[j] != 0) {       //checks that the buffer is not empty.
                        isEmpty = false;
                        break;
                    }
                }
        
                char newname[18]; 
                sprintf(newname, "recovered2-%s.txt", name); 

                if (!isEmpty) {
                    FILE* file = fopen(newname, "w");
                    fputs(buffer + 1, file);        //saves data found in sector on real filesystem.
                    fclose(file);
                }
            } 
        }
        dir_close(dir);
    }
}


