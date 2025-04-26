// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>

int main(int argc, char* argv[]) {
    int c = getpid();
    _exit(c * 1000);
}