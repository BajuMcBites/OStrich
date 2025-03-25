#include "fs.h"
#include "libk.h"
#include "ramfs.h"
#include "event.h"


/**
 * kread should read a file into the specified buffer by routing a read call to the
 * correct filesystem handler, ie userspace main fs, device read
 */
void read(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {


    int path_length = K::strnlen(file_name, PATH_MAX - 1);
    char* file_name_cpy = (char*) kmalloc(path_length + 1);
    K::strncpy(file_name_cpy, file_name, PATH_MAX);

    file_name = file_name_cpy;

    char* next;

    auto work = [=](int ret) {
        kfree(file_name_cpy);
        create_event<int>(w, ret);
    };

    if (file_name == nullptr) {
        create_event<int>(work, INVALID_FILE);
        return;
    }

    if (*file_name == '/') { /* starting from root */
        file_name++;
        next = K::strntok(file_name, '/', ENTRY_MAX + 1);

        if (next == nullptr)  {
            create_event<int>(work, INVALID_FILE);
            return;
        }

        if (K::strncmp(file_name, "dev", ENTRY_MAX) == 0) {
            read_dev(next, offset, buf, n, work);
            return;
        } else {
            create_event<int>(work, NOT_IMPLEMENTED_FS);
            return;
        }
    } else { /* starting from somewhere else */
        /* TODO */
        K::assert(false, "non root fs not implemented");
    }

}

/**
 * should take in the name of the file without /dev/, so "/dev/ramfs/temp.txt" would be 
 * passed in as "ramfs/temp.txt"
 */
void read_dev(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {

    int err;
    char* next = K::strntok(file_name, '/', ENTRY_MAX);

    if (K::strncmp(file_name, "ramfs", ENTRY_MAX) == 0) { /* ramfs file */
        int ramfs_index = get_ramfs_index(next);
        if (ramfs_index == -1) {
            create_event<int>(w, INVALID_FILE);
            return;
        }

        uint64_t file_size = ramfs_size(ramfs_index);
        if (offset >= file_size) {
            create_event<int>(w, INVALID_OFFSET);
            return;
        }

        int64_t read_n = K::min(n, file_size - offset);

        err = ramfs_read(buf, offset, read_n, ramfs_index);
        if (err != 0) {
            create_event<int>(w, OTHER_FAIL);
            return; 
        }

        create_event<int>(w, SUCCESSFUL_READ);
        return; /* done reading from ramfs */

    } else {
        create_event<int>(w, NOT_IMPLEMENTED_FS);
        return;
    }

}