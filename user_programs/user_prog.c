// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>

int main() {
    char* nul = "\0";
    char* argv[1] = {nul};
    execve("exit", argv, 0);
    return 0;
}