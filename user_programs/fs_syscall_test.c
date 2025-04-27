#include <fcntl.h>
#include <stdio.h>

int main() {
    FILE* fd = fopen("test.txt", "w");
    if (fd == NULL) {
        printf("Failed to open file\n");
        return 1;
    }

    // fwrite("Hello, world!\n", 1, 14, fd);
    // fclose(fd);

    // FILE* fd2 = fopen("test.txt", "r");
    // if (fd2 == NULL) {
    //     printf("Failed to open file\n");
    //     return 1;
    // }

    // char buffer[14];
    // fread(buffer, 1, 14, fd2);
    // printf("Read: %s\n", buffer);
    // fclose(fd2);

    return 0;
}