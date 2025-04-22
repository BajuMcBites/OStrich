#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    for (int i = 0; i < 20; i++) {
        int c = fork();
        if (c == 0) {
            char* str= "0\0";
            str[0] += i;
            // char* nargv[2] = {str, "\0"};
            // execve("exit", nargv, 0);
            _exit(10200);
        } else {
            int sig = 0;
            wait(&sig);
        }
    }
    _exit(0);
}