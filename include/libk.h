#ifndef _LIBK_H_
#define _LIBK_H_

#include <stdarg.h>
#include <stdint.h>

extern "C" void* memcpy(void* dest, const void* src, size_t n);

namespace K {
constexpr long strlen(const char* str) {
    long n = 0;
    while (*str++ != 0) n++;
    return n;
}
int isdigit(int c);
bool streq(const char* left, const char* right);
int strcmp(const char* stra, const char* strb);
int strncmp(const char* stra, const char* strb, int n);
int strnlen(char* str, int n);
int strncpy(char* dest, const char* src, int n);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
void* memcpy(void* dest, const void* src, int n);
void* memset(void* buf, unsigned char val, unsigned long n);
char* strntok(char* str, char c, int n);

template <typename T>
T&& move(T&& arg) {
    return static_cast<T&&>(arg);
}

bool check_stack();

template <typename T>
T min(T v) {
    return v;
}

template <typename T, typename... More>
T min(T a, More... more) {
    auto rest = min(more...);
    return (a < rest) ? a : rest;
}

void assert(bool condition, const char* msg);
};  // namespace K

#endif
