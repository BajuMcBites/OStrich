// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>

int main() {
    pid_t pid = getpid();
    return pid;
}