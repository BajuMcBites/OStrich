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
// Used by kopen and kclose.
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
class KFile {
   public:
    KFile() : file_type(FileType::UNKNOWN), offset(0) {
    }
    virtual ~KFile() = default;
    virtual int get_inode_number() = 0;

    FileType file_type;
    int offset;
};

class FSFile : public KFile {
   public:
    FSFile() : inode(nullptr), perm(0) {
        file_type = FileType::FILESYSTEM;
    }

    int get_inode_number() override {
        return inode->inode_number;
    }

    mem_inode_t* inode;
    int perm;
};

class DeviceFile : public KFile {
   public:
    DeviceFile() {
        file_type = FileType::DEVICE;
    }

    int get_inode_number() override {
        return 69696969696;
    }

    string name;
};

void kopen(string file_name, Function<void(KFile*)> w);
void kclose(KFile* file);

void kread(KFile* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void kwrite(KFile* file, uint64_t offset, const char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */