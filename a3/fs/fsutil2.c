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

    if (res == 1 && !(spaceavailable < 2)) {
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
  // TODO
}

int defragment() {
  // TODO
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