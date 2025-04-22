#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int c = fork();
    fork();
    fork();

    int check_copy = 0;

    if (c == 0) {
        if (check_copy != 0) {
            (*NULL);
        }
        check_copy = 1;
    } else {
        if (check_copy != 0) {
            (*NULL);
        }
        check_copy = 1;
    }

    check_copy = 0;

    int d = fork();
    int e = fork();

    check_copy = 1;

    if (check_copy != 1) {
        (*NULL);
    }

    check_copy = 0;
    char* str= "0\0";
    str[0] += getpid();
    char* nargv[2] = {str, "\0"};
    execve("exit", nargv, 0);
}