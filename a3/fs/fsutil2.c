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
    long fileSize;
    int bytesWritten = 0;


    source = fopen(fname, "r");
    if (!source) return 1;

    fseek(source, 0, SEEK_END);
    fileSize = ftell(source);
    rewind(source);
    
    int res = fsutil_create(fname, fileSize); 
    int spaceavailable = fsutil_freespace();

    int space_needed = (fileSize / 512) + 2;
    printf("DEBUG : space needed = %i\n", space_needed);


    if (res == 1 && !(spaceavailable < space_needed)) {
        char buffer[1024]; 
        int bytesRead = 0;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            int writeResult = fsutil_write(fname, buffer, bytesRead);
            bytesWritten += writeResult;
            if (writeResult < bytesRead) {
                printf("Warning: could only write %d out of %ld bytes (reached end of file)\n", bytesWritten, fileSize);
                break; 
            }
        }

        fclose(source);

        if (bytesWritten < fileSize) {
            return 3;
        } else {
            return 0;
        }
    } else {
      return 2;
    }
}

int copy_out(char *fname) {

    int size = fsutil_size(fname);
    char* buffer =  malloc((size+1) * sizeof(char));
    memset(buffer, 0, size + 1);

    struct file *f = get_file_by_fname(fname);
    int offset = file_tell(f);

    fsutil_seek(fname, 0);
    fsutil_read(fname, buffer, size);

    FILE* file = fopen(fname, "w");
    if (file == NULL) {
      fsutil_seek(fname, offset);    // take pointer back to original place.
      return 1;
    }
    fputs(buffer, file);
    fclose(file);
    
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
      struct file* f = get_file_by_fname(name);
      struct inode* node = file_get_inode(f);
      
      size_t num_sectors  = bytes_to_sectors(node->data.length);
      int truesize = (int) num_sectors;
      printf("size = %i\n", truesize);


      if (truesize > 1) {
      
        n_fragmentable++;

        block_sector_t *blocks = node->data.direct_blocks;
        // might have to also check indirect_block and doubly_indirect_block

        for (int i = 0; i < sizeof(blocks); i++) {
          printf("block[i] = %i\n", blocks[i]);
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

    float degree = n_fragmented / n_fragmentable;

    printf("Degree of fragmentation is : %f\n", degree);
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
      size_of_all_files = size_of_all_files + fsutil_size(name);
      n_files++;
    }
    dir_close(dir);


    char* buffer =  malloc((size_of_all_files+1 + n_files * 14) * sizeof(char));

    printf("size of buffer = %i\n ", (size_of_all_files+1 + n_files * 14));

    struct dir *dir2;
    char fname[NAME_MAX + 1];
    dir2 = dir_open_root(); 
    while (dir_readdir(dir2, fname)) {
    
      int size = fsutil_size(name);
      fsutil_seek(fname, 0);
      fsutil_read(fname, buffer, size); 
      strcat(buffer, "end_of_file");
    }

    char* start = buffer;
    char* end = NULL;
    int delimiterLen = strlen("end_of_file");

    char* parts[n_files];
    int partsCount = 0;

    while ((end = strstr(start, "end_of_file")) != NULL) {
        int partSize = end - start;
        parts[partsCount] = (char*)malloc(partSize + 1);

        strncpy(parts[partsCount], start, partSize);
        parts[partsCount][partSize] = '\0'; 

        start = end + delimiterLen; 
        partsCount++;
    }

    if (*start) {
        int partSize = strlen(start);
        parts[partsCount] = (char*)malloc(partSize + 1); 

        strcpy(parts[partsCount], start);
        partsCount++;
    }

    // Example: Print and free the parts
    for (int i = 0; i < partsCount; i++) {
        printf("File %d: %s\n", i + 1, parts[i]);
        free(parts[i]);
    }

  return 0;
}

void recover(int flag) {
  if (flag == 0) { // recover deleted inodes

    // TODO
  } else if (flag == 1) { // recover all non-empty sectors

    // TODO
  } else if (flag == 2) { // data past end of file.

    // TODO
  }
}