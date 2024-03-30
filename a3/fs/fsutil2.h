#ifndef FILESYS_FSUTIL2_H
#define FILESYS_FSUTIL2_H

//Karl WEHBE - 261085350
struct newfiles { 
    char* content;      // contains the content of a file
    char* name;         // contains the name of a file
    int size;           // contains the size of a file
};

int copy_in(char *fname);
int copy_out(char *fname);
void find_file(char *pattern);
void fragmentation_degree();
int defragment();
void recover(int flag);

#endif /* fs/fsutil2.h */
