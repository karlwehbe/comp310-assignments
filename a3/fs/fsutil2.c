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




int copy_in(char *fname) {
    FILE *source = fopen(fname, "r");
    if (!source) return 1;

    fseek(source, 0, SEEK_END);
    int fileSize = ftell(source);
    rewind(source);

    int spaceavailable = fsutil_freespace();
    int newSize = 0;

    int bytesavailable = 0;
    
    if (spaceavailable > 0) {
        if (fileSize <= 123*512) { bytesavailable = (spaceavailable - 1) * 512; }
        
        else if (fileSize <= 123*512 + 128*512) { bytesavailable = (spaceavailable - 2) * 512; }
            
        else if (fileSize > 123*512 + 128*512) { bytesavailable = (spaceavailable - 17) * 512 - 1; }

        if (bytesavailable > fileSize) {
            newSize = fileSize;
        } else { 
            newSize = bytesavailable;
        }
    }

    int res = fsutil_create(fname, 20);
    if (res != 1) {
        fclose(source);
        return 2;
    }

    if (newSize < fileSize) {
        printf("Warning: could only write %d out of %i bytes (reached end of file)\n", newSize + 1, fileSize);
    }

    char buffer[newSize];
    for (int i = 0; i < newSize; i++) {
        buffer[i] = fgetc(source);
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

    while (dir_readdir(dir, name)) {
        int size = fsutil_size(name);
        char* buffer =  malloc((size+1) * sizeof(char));
        memset(buffer, 0, size + 1);
        fsutil_read(name, buffer, size);

        fsutil_seek(name, 0);
        fsutil_read(name, buffer, size);

        if (strstr(buffer, pattern)) {
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

    while (dir_readdir(dir, name)) {
        struct file* f = filesys_open(name);
        struct inode* node = file_get_inode(f);
        
        int sectors_to_read = 0;
        if (fsutil_size(name) % 512 != 0) {
            sectors_to_read = (fsutil_size(name)/512) + 1;
        } else {
            sectors_to_read = (fsutil_size(name)/512);
        }
    
        if (sectors_to_read > 1) {
            n_fragmentable++;
            block_sector_t *blocks = get_inode_data_sectors(node);
            for (int i = 0; i < sectors_to_read; i++) {
                int place = 0;
                if (i == 0) {
                place = blocks[0];
                } else {
                place = blocks[i-1];
                }

                if (blocks[i] - place > 3 && blocks[i] != 0) {
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
          n_files++;
    }
    dir_close(dir);

    if (n_files == 0) {
        return 0;
    }

    struct newfiles* files = malloc(n_files * sizeof(struct newfiles));
    if (files == NULL) {
        return -1;
    }

    struct dir *dir2 = dir_open_root();
    char fname[NAME_MAX + 1];
    int i = 0;
    while (dir_readdir(dir2, fname) && i < n_files) {
        int size = fsutil_size(fname);
        files[i].content = malloc(size + 4);
        files[i].name = strdup(fname);
        files[i].size = size;

        struct file* f = filesys_open(fname);
        file_seek(f, 0);
        file_read(f, files[i].content, size);
        i++;
    }
    dir_close(dir2);

    for (int i = 0; i < n_files; i++) {
        fsutil_rm(files[i].name);
    }
  
    for (int i = 0; i < n_files; i++) {
        fsutil_create(files[i].name, files[i].size);
        fsutil_write(files[i].name, files[i].content, files[i].size);
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
                bitmap_set(free_map, i, 1);
                freesectors++;

                int sectors_to_read = 0;
                if (id.length % 512 != 0) {
                    sectors_to_read = i + (id.length/512 + 1);
                } else {
                    sectors_to_read = i + (id.length/512);
                }

                for (int j = i; j < sectors_to_read; j++) {
                    bitmap_set(free_map, j, 1);
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
  
        for (int i = 4; i < bitmap_size(free_map); i++) {

            if (bitmap_test(free_map, i)) {    // If the i-th bit is 1, gives 1.
                char buffer[1024];
                char newname[18]; 
                sprintf(newname, "recovered1-%u.txt", i); 
                memset(buffer, 0, sizeof(buffer)); 
                buffer_cache_read(i, buffer); 
            
                bool isEmpty = true;
                for (size_t j = 0; j < sizeof(buffer); j++) {
                    if (buffer[j] != 0) {
                        isEmpty = false;
                        break;
                    }
                }

                if (!isEmpty) {
                    FILE* file = fopen(newname, "w");
                    fputs(buffer, file);
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
            if (fsutil_size(name) % 512 != 0) {
                sectors_to_read = (fsutil_size(name)/512) + 1;
            } else {
                sectors_to_read = (fsutil_size(name)/512);
            }
      
            block_sector_t *blocks = get_inode_data_sectors(node); 
            int last_block = 0;
            for (int i = 0; i < sectors_to_read; i++) {
                if (blocks[i] != 0) {
                    last_block = blocks[i];
                }
            }

            if (bitmap_test(free_map, last_block)) {
                int remaining_bytes = 512 - (fsutil_size(name) % 512);
                int file_sectorbytes = 512 - remaining_bytes;
                
                printf("sectorstoread = %i, lastblock = %i and remaining_bytes = %i and filebytes = %i\n", sectors_to_read, last_block, remaining_bytes, file_sectorbytes);

                char buffer[512];
                memset(buffer, 0, sizeof(buffer)); 
                buffer_cache_read(last_block, buffer); 
                memmove(buffer, buffer + file_sectorbytes, sizeof(buffer) - file_sectorbytes); 
                
                bool isEmpty = true;
                for (size_t j = 0; j < sizeof(buffer); j++) {
                    if (buffer[j] != 0) {
                        isEmpty = false;
                        break;
                    }
                }
        
                char newname[18]; 
                sprintf(newname, "recovered2-%s.txt", name); 

                if (!isEmpty) {
                    FILE* file = fopen(newname, "w");
                    fputs(buffer + 1, file);
                    fclose(file);
                }
            } 
        }
        dir_close(dir);
    }
}


