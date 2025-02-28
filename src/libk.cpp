#include "libk.h"

#include "heap.h"
#include "printf.h"
#include "stdint.h"

long K::strlen(const char *str) {
    long n = 0;
    while (*str++ != 0) n++;
    return n;
}

int K::isdigit(int c) {
    return (c >= '0') && (c <= '9');
}

bool K::streq(const char *a, const char *b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

// Helper function for basic assertions
void K::assert(bool condition, const char *msg) {
    if (!condition) {
        printf("Assertion failed: ");
        printf(msg);
        printf("\n");
        while (1) {
        }  // Halt on failure
    }
}

/*****************/
/* C++ operators */
/*****************/
// typedef long unsigned int size_t;

void *operator new(size_t size) {
    void *p = malloc(size);
    // if (p == 0) Debug::panic("out of memory");
    return p;
}

void operator delete(void *p) noexcept {
    return free(p);
}

void operator delete(void *p, size_t sz) {
    return free(p);
}

void *operator new[](size_t size) {
    void *p = malloc(size);
    // if (p == 0) Debug::panic("out of memory");
    return p;
}

void operator delete[](void *p) noexcept {
    return free(p);
}

void operator delete[](void *p, size_t sz) {
    return free(p);
}
