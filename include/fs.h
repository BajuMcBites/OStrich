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
#include "string.h"

class mem_inode_t {
   public:
    mem_inode_t(uint64_t inode_number, uint64_t refs) : inode_number(inode_number), refs(refs) {
    }
    void increment_ref_count_atomic(uint64_t delta) {
        LockGuard<SpinLock> guard(lock);
        refs += delta;
    }

    SpinLock lock;
    uint64_t inode_number;
    uint64_t refs;
};

// Serves as a cache that maps from a minified file name to inode.
// Used by kfopen and kfclose.
class FileListNode {
   public:
    FileListNode(mem_inode_t* inode, string file_name)
        : inode(inode), file_name(K::move(file_name)), next(nullptr) {
        file_hash = this->file_name.hash();
    }
    mem_inode_t* inode;
    string file_name;
    uint64_t file_hash;
    FileListNode* next;
};

enum class FileType {
    FILESYSTEM,
    DEVICE,
    SWAP,
    UNKNOWN,
};

// Represents a file that is opened by any process (user or kernel).
class File {
   public:
    File() : inode(nullptr), file_type(FileType::UNKNOWN) {
    }
    mem_inode_t* inode;  // TODO: remove dependency on inode and use fileListNode instead.
    FileType file_type;
    string name;
};

void kfopen(string file_name, Function<void(File*)> w);
void kfclose(File* file);
void get_file_name(File* file, Function<void(char*)> w);

void kread(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void kwrite(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void read_dev(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */