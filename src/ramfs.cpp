#include "ramfs.h"

#include "libk.h"
#include "printf.h"

uint64_t ramfs_num_files;
ramfs_file* ramfs_dir_start;
char* ramfs_files_start;

void init_ramfs() {
    char* ptr = (char*)ramfs_start;
    uint64_t singature = *((uint64_t*)ptr);
    K::assert(singature == ramfs_signature, "loaded ramfs does not match signautre");

    ptr = ptr + 8;  // number of files
    ramfs_num_files = *((uint64_t*)ptr);

    ptr = ptr + 8;  // start of ramfs_file structs
    ramfs_dir_start = (ramfs_file*)ptr;

    ramfs_files_start = ptr + (sizeof(ramfs_file) * ramfs_num_files);
}

int get_ramfs_index(char* file_name) {
    for (uint64_t i = 0; i < ramfs_num_files; i++) {
        if (K::strncmp(file_name, ramfs_dir_start[i].file_name, 16) == 0) {
            return i;
        }
    }
    return -1;
}

uint64_t ramfs_size(int file_index) {
    K::assert(file_index < ramfs_num_files, "accesssing invalid file index on size\n");
    ramfs_file* file = ramfs_dir_start + file_index;
    return file->size;
}

int ramfs_read(void* buffer, uint64_t offset, uint64_t nbytes, int file_index) {
    K::assert(file_index < ramfs_num_files, "accesssing invalid file index on read\n");
    ramfs_file* file = ramfs_dir_start + file_index;

    K::assert(offset + nbytes <= file->size, "reading bytes that aren't a part of the file\n");
    char* read_ptr = ramfs_files_start + file->start + offset;
    K::memcpy(buffer, read_ptr, nbytes);
    return 0;
}