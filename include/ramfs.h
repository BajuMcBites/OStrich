#ifndef _RAMFS_H
#define _RAMFS_H

#include "stdint.h"

#define ramfs_start _binary_build_ramfs_img_start
#define ramfs_signature 0xf99482ea8f5ecce2

extern char _binary_build_ramfs_img_start[];

struct ramfs_file {
    char file_name[16];
    uint64_t size;
    uint64_t start;
};

extern uint64_t ramfs_num_files;
extern ramfs_file* ramfs_dir_start;
extern char* ramfs_files_start;

/**
 * checks a valid ramfs was loaded and initializes ramfs specific values
 */
void init_ramfs();

/**
 * gets the index of a ramfs file in the directory, returns the index, -1 if it
 * isn't in the directory
 */
int get_ramfs_index(char* file_name);

/**
 * gets the size of a ramfs file
 */
uint64_t ramfs_size(int file_index);

/**
 * reads nbytes into buffer starting at offset
 */
int ramfs_read(void* buffer, uint64_t offset, uint64_t nbytes, int file_index);
#endif