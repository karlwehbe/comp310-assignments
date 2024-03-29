#ifndef FILESYS_FSUTIL2_H
#define FILESYS_FSUTIL2_H

struct newfiles { 
    char* content;
    char* name;
    int size;
};

int copy_in(char *fname);
int copy_out(char *fname);
void find_file(char *pattern);
void fragmentation_degree();
int defragment();
void recover(int flag);

#endif /* fs/fsutil2.h */
