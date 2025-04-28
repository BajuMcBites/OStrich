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

enum class FileType {
    FILESYSTEM,
    DEV_RAMFS,
    SWAP,
    UNKNOWN,
};
// Represents a file that is opened by any process (user or kernel).
class KFile {
   public:
    KFile() : file_type(FileType::UNKNOWN), offset(0), ref_count(1) {
    }

    void increment_ref_count_atomic() {
        adjust_ref_count_atomic(1, /* no-op */ []() {});
    }

    void decrement_ref_count_atomic() {
        adjust_ref_count_atomic(-1, [this]() { delete this; });
    }

    uint64_t get_ref_count() const {
        return ref_count;
    }

    virtual ~KFile() = default;
    virtual int get_inode_number() = 0;

    FileType file_type;

   private:
    void adjust_ref_count_atomic(uint64_t delta, Function<void()> on_zero_refs) {
        lock.lockAndRelease([=]() mutable {
            ref_count += delta;
            if (ref_count == 0) {
                on_zero_refs();
            }
        });
    }
    int offset;
    int ref_count;
    Lock lock;
};

class FSFile : public KFile {
   public:
    FSFile() : inode_number(-1), perm(0) {
        file_type = FileType::FILESYSTEM;
    }
    FSFile(int inode_number, int perm) : inode_number(inode_number), perm(perm) {
        file_type = FileType::FILESYSTEM;
    }

    int get_inode_number() override {
        return inode_number;
    }

   private:
    int inode_number;
    int perm;
};

class DeviceFile : public KFile {
   public:
    DeviceFile(string name) : name(name) {
        file_type = FileType::DEV_RAMFS;  // for now.
    }

    int get_inode_number() override {
        return 6969;
    }

    string get_name() const {
        return name;
    }

   private:
    string name;
};

// Serves as a cache that maps from a minified file name to inode.
// Used by kopen and kclose.
class FileListNode {
   public:
    FileListNode(KFile* file, uint64_t hash) : file(file), name_hash(hash), next(nullptr) {
    }
    KFile* file;
    uint64_t name_hash;
    FileListNode* next;
};

void kopen(string file_name, Function<void(KFile*)> w);
void kclose(KFile* file);

void kread(KFile* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);
void kwrite(KFile* file, uint64_t offset, const char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */