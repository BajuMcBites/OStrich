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

class inode {
   public:
    inode(uint64_t inode_number, uint64_t refs) : inode_number(inode_number), refs(refs) {
    }
    void increment_ref_count_atomic(uint64_t delta) {
        LockGuard<SpinLock> guard(lock);
        refs += delta;
    }

    SpinLock lock;
    uint64_t inode_number;
    uint64_t refs;
};

class file {
   public:
    file() : inode(nullptr) {
    }
    inode* inode;
};

// Serves as a cache that maps from file name to inode.
class inodeListNode {
   public:
    inodeListNode(inode* inode, char* file_name)
        : inode(inode), file_name(file_name), next(nullptr) {
    }
    inode* inode;
    char* file_name;
    inodeListNode* next;
};

void kfopen(char* file_name, Function<void(file*)> w);
void kfclose(file* file);
void get_file_name(file* file, Function<void(char*)> w);

void read(file* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void read_dev(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */