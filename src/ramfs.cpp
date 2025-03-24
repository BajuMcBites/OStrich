#include "ramfs.h"

#include "libk.h"
#include "printf.h"
#include "mm.h"

uint64_t ramfs_num_files;
ramfs_file* ramfs_dir_start;
char* ramfs_files_start;

static inline uint64_t get_unaligned_64(const char *ptr) {
    uint64_t value = 0;
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        value |= ((uint64_t)(unsigned char)ptr[i]) << (8 * i);
    }
    return value;
}

void init_ramfs() {
    char *ptr = (char*)ramfs_start;

    // Use the helper function to safely load the 64-bit signature.
    uint64_t signature = get_unaligned_64(ptr);
    K::assert(signature == ramfs_signature, "loaded ramfs does not match signature");

    ptr += 8;  // move past the signature

    // Load the number of files.
    ramfs_num_files = get_unaligned_64(ptr);

    ptr += 8;  // move past the number of files field

    // The directory entries start at this point.
    ramfs_dir_start = (ramfs_file*)ptr;

    // The files start right after the directory entries.
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
    K::assert(file_index < ramfs_num_files, "accesssing invalid file index on size");
    ramfs_file* file = ramfs_dir_start + file_index;
    return file->size;
}

int ramfs_read(void* buffer, uint64_t offset, uint64_t nbytes, int file_index) {
    K::assert(file_index < ramfs_num_files, "accesssing invalid file index on read");
    ramfs_file* file = ramfs_dir_start + file_index;

    K::assert(offset + nbytes <= file->size, "reading bytes that aren't a part of the file");
    char* read_ptr = ramfs_files_start + file->start + offset;
    K::memcpy(buffer, read_ptr, nbytes);
    return 0;
}