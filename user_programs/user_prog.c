// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>

int main() {
    pid_t c = fork();
    if (c == 0) {
        char* nul = "\0";
        char* argv[1] = {nul};
        execve("exit", argv, 0);
    } else {
        while (1);
    }
    return 0;
}