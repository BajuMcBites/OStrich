#ifndef _LIBK_H_
#define _LIBK_H_

#include <stdarg.h>
#include <stdint.h>

extern "C" void* memcpy(void* dest, const void* src, size_t n);

class K {
   public:
    static long strlen(const char* str);
    static int isdigit(int c);
    static bool streq(const char* left, const char* right);
    static int strcmp(const char* stra, const char* strb);
    static int strncmp(const char* stra, const char* strb, int n);
    static int strnlen(char* str, int n);
    static int strncpy(char* dest, const char* src, int n);
    static char* strcpy(char* dest, const char* src);
    static char* strcat(char* dest, const char* src);
    static void* memcpy(void* dest, const void* src, int n);
    static void* memset(void* buf, unsigned char val, unsigned long n);
    static char* strntok(char* str, char c, int n);

    static bool check_stack();

    template <typename T>
    static T min(T v) {
        return v;
    }

    template <typename T, typename... More>
    static T min(T a, More... more) {
        auto rest = min(more...);
        return (a < rest) ? a : rest;
    }

    static void assert(bool condition, const char* msg);
};

#endif
