#include <fcntl.h>
#include <stdio.h>

extern int _open(const char* pathname, int flags, mode_t mode);
extern int _close(int fd);
extern int _write(int fd, const void* buf, size_t count);
extern void _exit(int status);
extern int _read(int fd, void* buf, size_t count);

int main() {
    int fd = _open("test.txt", O_WRONLY | O_CREAT, 0);
    _write(fd, "Hello, world!\n", 14);
    _close(fd);
    _exit(1);
    return 0;
}