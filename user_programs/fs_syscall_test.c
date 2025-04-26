#include <fcntl.h>

int main() {
    int fd = open("test.txt", O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        printf("Failed to open file\n");
        return 1;
    }

    write(fd, "Hello, world!\n", 14);
    close(fd);
}