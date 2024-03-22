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


void recreate_files(char **parts, int partsCount);

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

    int space_needed = (fileSize / 512) + 2;
    printf("DEBUG : size = %i and space needed = %i\n", fileSize, space_needed);


    if (res == 1 && !(spaceavailable < space_needed)) {
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

        printf("bits written = %i\n ", bitsWritten);

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
    if (f == NULL) {
      return 1;
    }
    int size = fsutil_size(fname);
    char* buffer =  malloc((size+1) * sizeof(char));
    memset(buffer, 0, size + 1);

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
      if (f == NULL) {
        printf("File %s not found.\n", name);
        continue; // Skip to the next iteration of the loop
      }

      printf("name = %s\n", name);
      struct inode* node = file_get_inode(f);
      if (node == NULL) {
        printf("Inode for %s not found.\n", name);
        continue; // Skip to the next iteration of the loop
      }
      
      size_t num_sectors  = bytes_to_sectors(node->data.length);
      int truesize = (int) num_sectors;
      printf("size = %i\n", truesize);


      if (truesize > 1) {
      
        n_fragmentable++;

        block_sector_t *blocks = node->data.direct_blocks;
        // might have to also check indirect_block and doubly_indirect_block

        for (int i = 0; i < sizeof(blocks); i++) {
          //printf("block[i] = %i\n", blocks[i]);
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

      printf("\nfilename = %s\n\n", fname);
      int size = fsutil_size(fname);
      char *text = malloc((size + 14) * sizeof(char));
      fsutil_seek(fname, 0);
      fsutil_read(fname, text, size);
      strcat(text, "\n\nEND_OF_FILE\n\n\n");

      strcat(buffer, fname);
      strcat(buffer, text);

      struct file *f = get_file_by_fname(fname);
      struct inode *node = file_get_inode(f);
      inode_remove(node);
  
    }

    dir_close(dir2);

    //free_map_close();   //deletes the whole memory.
    printf("space after freeing = %i\n", fsutil_freespace());


    char* start = buffer;
    char* end = NULL;
    int delimiterLen = strlen("END_OF_FILE");

    char* parts[n_files];
    int partsCount = 0;

    while ((end = strstr(start, "END_OF_FILE")) != NULL) {
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
        printf("File %d : %s\n", i + 1, parts[i]);
        free(parts[i]);
    }

    recreate_files(parts, partsCount);
  
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


void recreate_files(char **parts, int partsCount) {
    for (int i = 0; i < partsCount; i++) {
        // Assuming the first line of each part is the filename
        char *part = parts[i];
        char *endOfFileName = strchr(part, '\n');
        if (endOfFileName == NULL) {
            printf("Error: Filename delimiter not found in part %d\n", i);
            continue;
        }

        // Calculate the filename length and content length
        int fileNameLength = endOfFileName - part;
        int contentLength = strlen(part) - (fileNameLength + 1); // +1 to skip the newline

        // Allocate and set up the filename string
        char *fname = (char *)malloc(fileNameLength + 1); // +1 for null terminator
        if (fname == NULL) {
            printf("Error: Failed to allocate memory for filename\n");
            continue;
        }
        strncpy(fname, part, fileNameLength);
        fname[fileNameLength] = '\0'; // Null-terminate the filename

        // Point to the start of the actual content
        char *content = endOfFileName + 1;

        // Create and write to the file
        fsutil_create(fname, contentLength);
        fsutil_write(fname, content, contentLength);

        printf("Recreated file: %s\n", fname);

        free(fname); // Free the allocated filename string
    }
}