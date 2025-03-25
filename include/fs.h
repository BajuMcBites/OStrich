#ifndef _FS_H
#define _FS_H

#define PATH_MAX 4096
#define ENTRY_MAX 256

#define SUCCESSFUL_READ 0
#define INVALID_FILE -2
#define INVALID_OFFSET -3
#define OTHER_FAIL -100
#define NOT_IMPLEMENTED_FS -99

#include "function.h"
#include "stdint.h"

void read(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

void read_dev(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w);

// void write_dev();

#endif /* _FS_H */