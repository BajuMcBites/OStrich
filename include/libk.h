#ifndef _LIBK_H_
#define _LIBK_H_

#include <stdarg.h>

class K {
   public:
    static long strlen(const char* str);
    static int isdigit(int c);
    static bool streq(const char* left, const char* right);
    static int strncmp(const char* stra, const char* strb, int n);
    static int strncpy(char* dest, char* src, int n);
    static void* memcpy(void* dest, const void* src, int n);
    static void* memset(void *buf, unsigned char val, unsigned long n);
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
