// #include <sys/wait.h>
#include <malloc.h>
#include <unistd.h>

static int hello = 10;

int main(int argc, char* argv[]) {
    int* a = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* b = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* c = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* d = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* e = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* f = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    int* g = malloc(1000);
    if (a == NULL) {
        _exit(-1);
    }
    *a = 5;
    _exit((uint32_t)g);
}