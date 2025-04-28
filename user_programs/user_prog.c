#include <sys/wait.h>
#include <unistd.h>

static int k = 200;

int main(int argc, char* argv[]) {
    _exit(k);
}