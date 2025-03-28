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

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    return K::memcpy(dest, src, n);
}

void* K::memcpy(void* dest, const void* src, int n) {
    void* d = dest;
    void* s = (void*)src;

    while (n >= 8) {
        *reinterpret_cast<uint64_t*>(d) = *reinterpret_cast<uint64_t*>(s);
        n -= 8;
        d += 8;
        s += 8;
    }

    while (n--) {
        *reinterpret_cast<char*>(d++) = *reinterpret_cast<char*>(s++);
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
extern char __stacks_start[];
extern char __stacks_end[];

/**
 * returns true if the current within its range, fails assert otherwise
 */
bool K::check_stack() {
    uint64_t sp = get_sp();
    uint64_t stack_size = 16384;

    uint64_t stack_end = (uint64_t)__stacks_start;

    stack_end += stack_size * 8 * (getCoreID());

    uint64_t stack_start = stack_end + stack_size * 8;

    if (sp <= stack_end || sp > stack_start) {
        printf("Stack (%d) has overflowed. end (%X), sp (%X), start (%X)\n", getCoreID(), stack_end,
               sp, stack_start);
        K::assert(false, "we have failed stack check");
    }

    return sp > stack_end;
}

/**
 * NOTE: not the same as linux strntok, returns the start of the next token
 *
 * Tokenizes strings, but not the same as linux strntok, this is stateless,
 * takes in a string, returns null if no instance of the char c in n characters
 * or we encounter a '\0' first. returns the start of the next token if there
 * is an occurance.
 */
char* K::strntok(char* str, char c, int n) {

    char* cpy = str;
    int count = 0;

    while (*cpy != '\0' && *cpy != c && count < n) {
        cpy++;
        count++;
    }

    if (*cpy == c) {
        *cpy = '\0';
        cpy++;
        return cpy;
    }

    return nullptr;
}

/**
 * returns the min of length of the string not including the null terminator and n
 */
int K::strnlen(char* str, int n) {
    char* cpy = str;
    int len = 0;

    while (*cpy != '\0' && len < n) {
        cpy++;
        len++;
    }

    return len;
}
