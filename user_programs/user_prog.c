// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int c = fork();
    if (c == 0) {
        _exit(120);
    } else {
        int status = 0;
        _wait(&status);
    }
    return 0;
}