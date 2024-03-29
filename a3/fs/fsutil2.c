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


    if (res == 1 && spaceavailable >= space_needed && fileSize <= bytesavailable) {
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

      //struct bitmap* bmap = free_map;
      //bitmap_mark(bmap, node->sector);
      
      int sectors_to_read = 0;
      if (fsutil_size(name) % 512 != 0) {
        sectors_to_read = (fsutil_size(name)/512) + 1;
      } else {
        sectors_to_read = (fsutil_size(name)/512);
      }

      //if (sectors_to_read == 1) {
       // block_sector_t *blocks = get_inode_data_sectors(node);
        //if (blocks[0] != 0) bitmap_mark(bmap, blocks[0]);
      //}
      //printf("sectors to read = %i\n", sectors_to_read);

      if (sectors_to_read > 1) {
      
        n_fragmentable++;
        
        block_sector_t *blocks = get_inode_data_sectors(node);

        for (int i = 0; i < sectors_to_read; i++) {
            //printf("filename = %s, size = %i, block[i] = %i\n", name, sectors_to_read, blocks[i]);
            int place = 0;
            if (i == 0) {
              place = blocks[0];
            } else {
              place = blocks[i-1];
            }

            //if (blocks[i] != 0) bitmap_mark(bmap, blocks[i]);
            
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
    //buffer_cache_close();
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
      if (size > 512) {
        size_of_all_files += fsutil_size(name) ;
        n_files++;
      }

    }
    dir_close(dir);

    if (n_files == 0) {
      return 0;
    }

    char** fnames = malloc(n_files * sizeof(char*));
    char* buffer =  malloc((size_of_all_files+1 + n_files * 16) * sizeof(char));
    buffer[0] = '\0';


    struct dir *dir2 = dir_open_root();
    char fname[NAME_MAX + 1];
    int i = 0;
    while (dir_readdir_inode(dir2, fname) && i < n_files) {
      
      int size = fsutil_size(fname);
      if (size > 512) {
        char text[size + strlen("\n\nEND_OF_FILE\n\n\n") + 1];
        fsutil_seek(fname, 0);
        fsutil_read(fname, text, size);
        strcat(text, "\n\nEND_OF_FILE\n\n\n");
        strcat(buffer, text);

        fnames[i] = strdup(fname);
        i++;
      }
    }
    dir_close(dir2);

    for (int i = 0; i < n_files; i++) {
      fsutil_rm(fnames[i]);
    }


    char* start = buffer;
    char* end = NULL;
    int delimiterLen = strlen("END_OF_FILE");
    char** parts = malloc(n_files * sizeof(char*) + size_of_all_files);

    if (!parts) {
        free(buffer);
        return -1;
    }

    int partsCount = 0;
    while ((end = strstr(start, "END_OF_FILE")) != NULL) {
        int partSize = end - start;
        parts[partsCount] = malloc(partSize + 1);
        if (!parts[partsCount]) {
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

    for (int i = 0; i < n_files; i++) {
        fsutil_create((const char*) fnames[i], strlen(parts[i])-1);
        fsutil_write(fnames[i], parts[i], strlen(parts[i])-1);
        struct file* f = get_file_by_fname(fnames[i]);
        add_to_file_table(f, fnames[i]);
    }
    free(buffer); 
    free(fnames);
    

    fragmentation_degree();

    return 0;
}



void recover(int flag) {
  if (flag == 0) {            // recover deleted inodes
   
    struct bitmap* bmap = free_map;
    int freesectors = 0;

    for (int i = 0; i < bitmap_size(bmap); i++) {

      if (!bitmap_test(bmap, i)) {    // If the i-th bit is 0 (free sector), gives 1 if removed/empty
          struct inode *node = inode_open(i);     //only gives an inode if its the sector of the inode, if represnts the data, it will not give back an inode.
          struct inode_disk id = node->data;
          struct file* f = file_open(node);

          if (id.length > 0 && !id.is_dir && node->sector > 0) {
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
            add_to_file_table(f, newname);
            
            struct dir* root = dir_open_root();
            dir_add(root, newname, i, false);
          }
          
      }
    }


  } else if (flag == 1) { // recover all non-empty sectors
    struct bitmap* bmap = free_map;
  
    for (int i = 4; i < bitmap_size(bmap); i++) {

        if (bitmap_test(bmap, i)) {    // If the i-th bit is 1, gives 1.
            struct inode *node = inode_open(i);     //only gives an inode if its the sector of the inode, if represnts the data, it will not give back an inode.
            struct inode_disk id = node->data;
            struct file* f = file_open(node);
            printf("sector = %i\n", node->sector);

            if (id.length > 0 && !id.is_dir && node->sector > 3 && node->removed == 1) {
              
              int sectors_to_read = 0;
              if (id.length % 512 != 0) {
                sectors_to_read = id.length/512 + 1;
              } else {
                sectors_to_read = id.length/512;
              }

              printf("before i=%i\n", i);
              i += sectors_to_read;
              printf("after i=%i\n", i);

              char newname[17]; 
              sprintf(newname, "recovered1-%u.txt", node->sector); 
              add_to_file_table(f, newname);
              printf("newname = %s\n", newname);

              copy_out(newname);
              fsutil_rm(newname);
            }
        }
    } 
    
  } else if (flag == 2) { // data past end of file.

    // TODO
  }
}


void read_indirect_block(block_sector_t indirect_block, char *buffer, int *offset) { 
  block_sector_t sectors[INDIRECT_BLOCKS_PER_SECTOR]; 
  buffer_cache_read(indirect_block, &sectors); 
  for (int i = 0; i < INDIRECT_BLOCKS_PER_SECTOR; i++) { 
    if (sectors[i] != 0) { 
      buffer_cache_read(sectors[i], buffer + *offset); 
      *offset += BLOCK_SECTOR_SIZE; } 
      } 
} 

void read_doubly_indirect_block(block_sector_t doubly_indirect_block, char *buffer, int *offset) { 
  block_sector_t indirect_blocks[INDIRECT_BLOCKS_PER_SECTOR]; 
  buffer_cache_read(doubly_indirect_block, &indirect_blocks); 
  for (int i = 0; i < INDIRECT_BLOCKS_PER_SECTOR; i++) { 
    if (indirect_blocks[i] != 0) { 
      read_indirect_block(indirect_blocks[i], buffer, offset); 
      } 
  }
} 