#include <fcntl.h>
#include <stdio.h>

extern int _open(const char* pathname, int flags, mode_t mode);
extern int _close(int fd);
extern int _write(int fd, const void* buf, size_t count);
extern void _exit(int status);
extern int _read(int fd, void* buf, size_t count);

void test_one_file() {
    int fd = _open("test.txt", O_WRONLY | O_CREAT, 0);
    _write(fd, "Hello, world!\n", 14);
    _close(fd);
}

void test_two_files() {
    int fd = _open("test2.txt", O_WRONLY | O_CREAT, 0);
    _write(fd, "Hello, world!\n", 14);
    _close(fd);

    int fd2 = _open("test2.txt", O_WRONLY, 0);
    _write(fd2, "Hello, world!\n", 14);
    _close(fd2);
}

void test_failure() {
    int fd = _open("test123123.txt", O_WRONLY, 0);
    if (fd != -1) {
        _exit(15151515);
    }
}

int main() {
    test_one_file();
    test_two_files();
    test_failure();
    _exit(0);
    return 0;
}