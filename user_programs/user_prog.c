// Simple user progam that uses libc but only calls one syscall.

#include <unistd.h>

int main(int argc, char* argv[]) {
    const int N_PROC = 1;
    int arr[N_PROC];
    for (int i = 0; i < N_PROC; i++) {
        arr[i] = fork();
        if (arr[i] == 0) {
            char str[2];
            str[0] = (char)('1' + i);
            str[1] = '\0';
            char* nargv[2] = {str, "\0"};
            execve("exit", nargv, 0);
        } else {
            continue;
        }
    }
    _exit(12345);
    // char str[2];
    // str[0] = (char)('1');
    // str[1] = '\0';
    // char* nargv[2] = {str, "\0"};
    // execve("exit", nargv, 0);
    return 0;
}