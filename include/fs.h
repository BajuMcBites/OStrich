#ifndef _FS_H
#define _FS_H

#define PATH_MAX 4096
#define ENTRY_MAX 256

#define SUCCESSFUL_READ 0
#define INVALID_FILE -2
#define INVALID_OFFSET -3
#define OTHER_FAIL -100
#define NOT_IMPLEMENTED_FS -99

#include "atomic.h"
#include "function.h"
#include "stdint.h"

struct inode;

struct file {
    struct inode* inode;
};

struct inode {
    SpinLock lock;
    uint64_t inode_number;
    uint64_t refs;
};

struct inodeListNode {
    struct inode* inode;
    char* file_name;
    inodeListNode* next;
};

void kfopen(char* file_name, Function<void(file*)> w);
void kfclose(file* file);
void get_file_name(file* file, Function<void(char*)> w);

void read(file* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void read_dev(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

void write(file* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */