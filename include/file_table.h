#ifndef _FILE_TABLE_H_
#define _FILE_TABLE_H_

#include "atomic.h"
#include "fs.h"
#include "function.h"

constexpr int MAX_OPEN_FILES = 1024;
constexpr int FD_START = 3;

#define TOO_MANY_OPEN_FILES -1
#define UNKNOWN_ERROR -2

// File mode flags that match C flags
enum OpenFlags {
    O_RDONLY = 0x0000,    // Open for reading only
    O_WRONLY = 0x0001,    // Open for writing only
    O_RDWR = 0x0002,      // Open for reading and writing
    O_APPEND = 0x0008,    // Append on each write
    O_CREAT = 0x0200,     // Create file if it does not exist
    O_TRUNC = 0x0400,     // Truncate flag
    O_EXCL = 0x0800,      // Exclusive use flag
    O_DIRECTORY = 0x0100  // Must be a directory
};

class UFile {
   public:
    UFile() : backing_file_(nullptr), offset_(0), mode_flags_(0), permissions_(0) {
    }
    UFile(KFile* file, int offset, int mode_flags)
        : backing_file_(file), offset_(offset), mode_flags_(mode_flags) {
    }
    void reset() {
        backing_file_ = nullptr;
        offset_ = 0;
        mode_flags_ = 0;
        permissions_ = 0;
    }
    ~UFile() {
        if (backing_file_) {
            kclose(backing_file_);
        }
    }
    void set_offset(int offset) {
        offset_ = offset;
    }
    int offset() {
        return offset_;
    }
    int mode_flags() {
        return mode_flags_;
    }
    KFile* backing_file() {
        return backing_file_;
    }
    void increment_offset(int bytes) {
        offset_ += bytes;
    }

   private:
    KFile* backing_file_;
    int offset_;
    int mode_flags_;
    int permissions_;
};

class FileTable {
   public:
    FileTable() : fd_counter_(FD_START), open_files_(0) {
    }
    ~FileTable() = default;
    void add_file(UFile& file, Function<void(int)> w);
    UFile& get_file(int fd);
    void remove_file(int fd, Function<void(int)> w);

   private:
    UFile file_table_[MAX_OPEN_FILES];
    Lock file_table_lock_;
    int fd_counter_;
    int open_files_;
};

#endif
