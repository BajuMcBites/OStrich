/*
File: printf.c

Copyright (C) 2004  Kustaa Nyholm

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "printf.h"

#include "atomic.h"
#include "framebuffer.h"
#include "uart.h"

typedef void (*putcf)(void*, char);
static putcf stdout_putf;
static void* stdout_putp;

SpinLock printf_err_lock;
SpinLock printf_lock;
SpinLock panic_lock;

#ifdef PRINTF_LONG_SUPPORT

static void uli2a(unsigned long int num, unsigned int base, int uc, char* bf) {
    int n = 0;
    unsigned int d = 1;
    while (num / d >= base) d *= base;
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0) {
            *bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
            ++n;
        }
    }
    *bf = 0;
}

static void li2a(long num, char* bf) {
    if (num < 0) {
        num = -num;
        *bf++ = '-';
    }
    uli2a(num, 10, 0, bf);
}

#endif

static void ui2a(unsigned int num, unsigned int base, int uc, char* bf) {
    int n = 0;
    unsigned int d = 1;
    while (num / d >= base) d *= base;
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0) {
            *bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
            ++n;
        }
    }
    *bf = 0;
}

static void i2a(int num, char* bf) {
    if (num < 0) {
        num = -num;
        *bf++ = '-';
    }
    ui2a(num, 10, 0, bf);
}

static int a2d(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

static char a2i(char ch, const char** src, int base, int* nump) {
    const char* p = *src;
    int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0) {
        if (digit > base) break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

static void putchw(void* putp, putcf putf, int n, char z, char* bf) {
    char fc = z ? '0' : ' ';
    char ch;
    char* p = bf;
    while (*p++ && n > 0) n--;
    while (n-- > 0) putf(putp, fc);
    while ((ch = *bf++)) putf(putp, ch);
}

static void printchar(void* putp, void (*putf)(void*, char), int c) {
    putf(putp, (char)c);
}

static void prints(void* putp, void (*putf)(void*, char), const char* str, int width, int pad) {
    int pc = 0;
    char padchar = ' ';
    int len = 0;
    const char* ptr;
    for (ptr = str; *ptr; ++ptr) ++len;
    if (width > len) {
        width -= len;
    } else {
        width = 0;
    }
    if (!(pad & 2)) {
        for (; width > 0; --width) {
            printchar(putp, putf, padchar);
            pc++;
        }
    }
    for (; *str; ++str) {
        printchar(putp, putf, *str);
        pc++;
    }
    for (; width > 0; --width) {
        printchar(putp, putf, padchar);
        pc++;
    }
}

/* printi(): prints an integer in a given base.
   'i' is the number, base is the numeral base,
   sign indicates whether to treat the value as signed,
   width and pad for formatting,
   letbase is either 'a' or 'A' for hex digit case.
   This routine supports 64-bit values.
*/
static void printi(void* putp, void (*putf)(void*, char), long long i, int base, int sign,
                   int width, int pad, int letbase) {
    char print_buf[65];
    char* s;
    int neg = 0;
    unsigned long long t;
    if (sign && i < 0) {
        neg = 1;
        t = (unsigned long long)(-i);
    } else {
        t = (unsigned long long)i;
    }
    s = print_buf + sizeof(print_buf) - 1;
    *s = '\0';
    if (t == 0) {
        *(--s) = '0';
    } else {
        while (t) {
            int rem = t % base;
            t /= base;
            if (rem >= 10) {
                rem += letbase - '0' - 10;
            }
            *(--s) = '0' + rem;
        }
    }
    if (neg) {
        if (pad & 1) {
            printchar(putp, putf, '-');
            if (width) width--;
        } else {
            *(--s) = '-';
        }
    }

    prints(putp, putf, s, width, pad);
}

void tfp_format(void* putp, void (*putf)(void*, char), const char* fmt, va_list va) {
    int width, pad;
    char scr[2];
    while (*fmt) {
        if (*fmt != '%') {
            putf(putp, *fmt++);
            continue;
        }
        fmt++; /* skip '%' */
        width = pad = 0;
        if (*fmt == '\0') break;
        if (*fmt == '%') {
            putf(putp, *fmt++);
            continue;
        }
        if (*fmt == '-') {
            fmt++;
            pad = 2;
        }
        while (*fmt == '0') {
            fmt++;
            pad |= 1;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        int long_flag = 0;      // flag for 'l'
        int longlong_flag = 0;  // flag for 'll'
        while (*fmt == 'l') {
            fmt++;
            if (long_flag) longlong_flag = 1;
            long_flag = 1;
        }
        switch (*fmt) {
            case 's': {
                const char* s = va_arg(va, const char*);
                if (!s) s = "(null)";
                prints(putp, putf, s, width, pad);
                break;
            }
            case 'd': {
                if (longlong_flag)
                    printi(putp, putf, va_arg(va, long long), 10, 1, width, pad, 'a');
                else if (long_flag)
                    printi(putp, putf, va_arg(va, long), 10, 1, width, pad, 'a');
                else
                    printi(putp, putf, va_arg(va, int), 10, 1, width, pad, 'a');
                break;
            }
            case 'u': {
                if (longlong_flag)
                    printi(putp, putf, va_arg(va, unsigned long long), 10, 0, width, pad, 'a');
                else if (long_flag)
                    printi(putp, putf, va_arg(va, unsigned long), 10, 0, width, pad, 'a');
                else
                    printi(putp, putf, va_arg(va, unsigned int), 10, 0, width, pad, 'a');
                break;
            }
            case 'x': {
                if (longlong_flag)
                    printi(putp, putf, va_arg(va, unsigned long long), 16, 0, width, pad, 'a');
                else if (long_flag)
                    printi(putp, putf, va_arg(va, unsigned long), 16, 0, width, pad, 'a');
                else
                    printi(putp, putf, va_arg(va, unsigned int), 16, 0, width, pad, 'a');
                break;
            }
            case 'X': {
                if (longlong_flag)
                    printi(putp, putf, va_arg(va, unsigned long long), 16, 0, width, pad, 'A');
                else if (long_flag)
                    printi(putp, putf, va_arg(va, unsigned long), 16, 0, width, pad, 'A');
                else
                    printi(putp, putf, va_arg(va, unsigned int), 16, 0, width, pad, 'A');
                break;
            }
            case 'c': {
                scr[0] = (char)va_arg(va, int);
                scr[1] = '\0';
                prints(putp, putf, scr, width, pad);
                break;
            }
            default:
                putf(putp, *fmt);
                break;
        }
        fmt++;
    }
}

void init_printf(void* putp, void (*putf)(void*, char)) {
    stdout_putf = putf;
    stdout_putp = putp;
}

static void putcp(void* p, char c) {
    *(*((char**)p))++ = c;
}

static void vtfp_sprintf(char* s, const char* fmt, va_list va) {
    tfp_format(&s, putcp, fmt, va);
    putcp(&s, 0);
}

void tfp_printf(const char* fmt, ...) {
    printf_lock.lock();
    char buffer[256];
    va_list va, va_copy_list;

    va_start(va, fmt);
    va_copy(va_copy_list, va);

    vtfp_sprintf(buffer, fmt, va);
    tfp_format(stdout_putp, stdout_putf, fmt, va_copy_list);

    va_end(va_copy_list);
    va_end(va);

    // fb_print(buffer, WHITE);
    printf_lock.unlock();
}

void tfp_sprintf(char* s, char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    tfp_format(&s, putcp, fmt, va);
    putcp(&s, 0);
    va_end(va);
}

void tfp_error_printf(const char* fmt, ...) {
    printf_err_lock.lock();
    char buffer[256];
    va_list va, va_copy;
    va_start(va, fmt);
    va_copy(va_copy, va);
    vtfp_sprintf(buffer, fmt, va);
    tfp_format(stdout_putp, stdout_putf, fmt, va_copy);
    va_end(va_copy);
    va_end(va);

    // print the buffer to the framebuffer in red
    // fb_print(buffer, RED);
    printf_err_lock.unlock();
}

void tfp_printf_no_lock(const char* fmt, unsigned int color, ...) {
    char buffer[256];
    va_list va, va_copy_list;

    va_start(va, fmt);
    va_copy(va_copy_list, va);

    vtfp_sprintf(buffer, fmt, va);
    tfp_format(stdout_putp, stdout_putf, fmt, va_copy_list);

    va_end(va_copy_list);
    va_end(va);

    fb_print(buffer, color);
}

__attribute__((noreturn)) void tfp_panic(const char* fmt, ...) {
    panic_lock.lock();

    char buffer[256];
    va_list va;
    va_start(va, fmt);
    vtfp_sprintf(buffer, fmt, va);
    va_end(va);
    tfp_printf_no_lock("\n***** KERNEL PANIC *****\n", RED);
    tfp_printf_no_lock("%s\n", RED, buffer);

    panic_lock.unlock();

    // Hang forever in low power state.
    while (1) {
        __asm__ volatile("wfi");
    }
}
