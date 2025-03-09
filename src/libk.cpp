#include "libk.h"

#include "heap.h"
#include "printf.h"
#include "stdint.h"
#include "utils.h"

long K::strlen(const char* str) {
    long n = 0;
    while (*str++ != 0) n++;
    return n;
}

int K::isdigit(int c) {
    return (c >= '0') && (c <= '9');
}

bool K::streq(const char* a, const char* b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

int K::strcmp(const char* stra, const char* strb) {
    int index = 0;
    while (1) {
        if (stra[index] == '\0' && strb[index] == '\0') return 0;
        else if (stra[index] < strb[index]) return -1;
        else if (stra[index] > strb[index]) return 1;
        index++;
    }
}

int K::strncmp(const char* stra, const char* strb, int n) {
    int index = 0;
    while (index < n && stra[index] != '\0' && strb[index] != '\0') {
        if (stra[index] != strb[index]) {
            return stra[index] - strb[index];
        }
        index++;
    }

    if (index == n) {
        return 0;
    }

    return stra[index] - strb[index];
}

int K::strncpy(char* dest, char* src, int n) {
    int index = 0;
    while (index < n && src[index] != '\0') {
        dest[index] = src[index];
        index++;
    }

    if (index == n) {
        dest[n - 1] = '\0';
        return n;
    }

    dest[index] = '\0';
    return index + 1;
}

void* K::memcpy(void* dest, const void* src, int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

void* K::memset(void* buf, unsigned char val, unsigned long n) {
    char* temp = (char*)buf;
    while (n > 0) {
        *temp++ = val;
        n--;
    }
    return buf;
}

// Helper function for basic assertions
void K::assert(bool condition, const char* msg) {
    if (!condition) {
        printf("Assertion failed: ");
        printf(msg);
        printf("\n");
        while (1) {
        }  // Halt on failure
    }
}