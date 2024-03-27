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

    // still need to add extreme cases, if no space in memory
    FILE *source;
    int fileSize;
    int bitsWritten = 0;


    source = fopen(fname, "r");
    if (!source) return 1;

    fseek(source, 0, SEEK_END);
    fileSize = ftell(source);
    rewind(source);
    
    int res = fsutil_create(fname, fileSize); 
    int spaceavailable = fsutil_freespace();

    int sectors_to_read = 0;
    if (fileSize % 512 != 0) {
      sectors_to_read = (fileSize/512) + 2;
    } else {
      sectors_to_read = (fileSize/512) + 1;
    }

    int space_needed = sectors_to_read;
    //printf("DEBUG : size = %i and space needed = %i\n", fileSize, space_needed);

    int bytesavailable = (spaceavailable - 1) * 512;

    int need_indirectblocks = 123*512;


    if (res == 1 && !(spaceavailable < space_needed) && fileSize < need_indirectblocks && fileSize < bytesavailable) {
        char buffer[fileSize+1]; 
        int bitsRead = 0;
        while ((bitsRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            int writeResult = fsutil_write(fname, buffer, bitsRead+1);
            bitsWritten += writeResult;
            if (writeResult < bitsRead) {
                printf("Warning: could only write %d out of %i bytes (reached end of file)\n", bitsWritten, fileSize);
                break; 
            }
        }

        //printf("bits written = %i\n ", bitsWritten);

        fclose(source);

        if (bitsWritten < fileSize) {
            return 3;
        } else {
            return 0;
        }
    } else {
      return 2;
    }
}

int copy_out(char *fname) {

    struct file *f = get_file_by_fname(fname);
    
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

      int size = fsutil_size(name);

      struct file* f = get_file_by_fname(name);
      struct inode* node = file_get_inode(f);
      
    
      int sectors_to_read = 0;
      if (size % 512 != 0) {
        sectors_to_read = (size/512) + 1;
      } else {
        sectors_to_read = (size/512);
      }

      if (sectors_to_read > 1) {
      
        n_fragmentable++;
        
        block_sector_t *blocks = get_inode_data_sectors(node);
        // might have to also check indirect_block and doubly_indirect_block

        for (int i = 0; i < sectors_to_read; i++) {
            //printf("filename = %s, size = %i, block[i] = %i\n", name, sectors_to_read, blocks[i]);
            int place = 0;
            if (i == 0) {
              place = blocks[0];
            } else {
              place = blocks[i-1];
            }
            
            if (blocks[i] - place > 3 && blocks[i] != 0) {
              n_fragmented++;
              //printf("fragmentable increased\n");
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
    if (dir == NULL)
      return 1;

    int size_of_all_files = 0;
    int n_files = 0;
    
    while (dir_readdir(dir, name)) {
      int size = fsutil_size(name);
      if (size > 0) {
        size_of_all_files += fsutil_size(name) ;
        n_files++;
      }
    }
    dir_close(dir);

    if (n_files == 0) {
      return 0;
    }


    const char** fnames = malloc(n_files * sizeof(char*));

    char* buffer =  malloc((size_of_all_files+1 + n_files * 16) * sizeof(char));
    buffer[0] = '\0';
    //printf("size of buffer = %i\n ", (size_of_all_files+1 + n_files * 14));

    
    struct dir *dir2 = dir_open_root();
    char fname[NAME_MAX + 1];
    int number = 0;

    while (dir_readdir(dir2, fname) && number < n_files) {

      int size = fsutil_size(fname);

      if (size > 0) {
        char text[size + strlen("\n\nEND_OF_FILE\n\n\n") + 1];
        fsutil_seek(fname, 0);
        fsutil_read(fname, text, size);
        strcat(text, "\n\nEND_OF_FILE\n\n\n");
        strcat(buffer, text);

        fnames[number] = (const char*)strdup(fname);
        number++;
        fsutil_rm(fname);

      }
    }
    dir_close(dir2);

    //printf("space after freeing = %i\n", fsutil_freespace());
   

    char* start = buffer;
    char* end = NULL;
    int delimiterLen = strlen("END_OF_FILE");

    char** parts = malloc(n_files * sizeof(char*));
    if (!parts) {
        free(buffer);
        return -1;
    }
    int partsCount = 0;

    while ((end = strstr(start, "END_OF_FILE")) != NULL) {
        int partSize = end - start;
        parts[partsCount] = malloc(partSize + 1);
        if (!parts[partsCount]) {
            // Handle malloc failure for any part
            for (int i = 0; i < partsCount; ++i) {
                free(parts[i]);
            }
            free(parts);
            free(buffer);
            return -1;
        }

        strncpy(parts[partsCount], start, partSize);
        parts[partsCount][partSize] = '\0'; 
        start = end + delimiterLen; 
        partsCount++;
    }

    if (*start) {
        int partSize = strlen(start);
        parts[partsCount] = malloc(partSize + 1); 
        if (!parts[partsCount]) {
            for (int i = 0; i < partsCount; ++i) {
                free(parts[i]);
            }
            free(parts);
            free(buffer);
            return -1;
        }

        strcpy(parts[partsCount], start);
        partsCount++;
    }

    for (int i = 0; i < partsCount - 1; i++) {
        fsutil_create(fnames[i], strlen(parts[i])-1);
        fsutil_write((char*)fnames[i], parts[i], strlen(parts[i])-1);
    }
    free(buffer); 
    free(fnames);

    return 0;
}



void recover(int flag) {
  if (flag == 0) {            // recover deleted inodes
   
    struct bitmap* bmap = free_map;

    //printf("bitmap size = %i\n", bitmap_size(bmap));
    int freesectors = 0;
    int size = 0;

    for (int i = 0; i < bitmap_size(bmap); i++) {

      if (!bitmap_test(bmap, i)) { // If the i-th bit is 0 (free sector), gives 1 if removed/empty
          struct inode *node = inode_open(i); //only gives an inode if its the sector of the inode, if represnts the data, it will not give back an inode.
          struct inode_disk id = node->data;
          struct file* f = file_open(node);

          if (id.length > 0 && !id.is_dir && node->sector > 0) {
            //printf(" at i = %i, sector of node = %i and length of node/file = %i \n", i, node->sector, id.length);
            
            node->removed = 0;
            bitmap_set(bmap, i, 1);
            freesectors++;

            int sectors_to_read = 0;
            if (id.length % 512 != 0) {
              sectors_to_read = i + (id.length/512 + 1);
            } else {
              sectors_to_read = i + (id.length/512);
            }

            for (int j = i; j < sectors_to_read; j++) {
                bitmap_set(bmap, j, 1);
                freesectors++;
            }

            char newname[15]; 
            sprintf(newname, "recovered0-%u", node->sector); 
            //printf("new name = %s\n", newname);
            add_to_file_table(f, newname);
            
            struct dir* root = dir_open_root();
            dir_add(root, newname, i, false);
            
          }
          
      }
      size++;
    }

    //printf("# of sectors reallocated = %i and after = %i and total loops = %i\n", freesectors, num_free_sectors(), size);


  } else if (flag == 1) { // recover all non-empty sectors

    // TODO
  } else if (flag == 2) { // data past end of file.

    // TODO
  }
}


  