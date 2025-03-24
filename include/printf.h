#ifndef __TFP_PRINTF__
#define __TFP_PRINTF__

#include <stdarg.h>

void init_printf(void* putp, void (*putf)(void*, char));

void tfp_printf(const char* fmt, ...);
__attribute__((noreturn)) void tfp_panic(const char* fmt, ...);
void tfp_sprintf(char* s, const char* fmt, ...);

void tfp_format(void* putp, void (*putf)(void*, char), const char* fmt, va_list va);
void tfp_error_printf(const char *fmt, ...);

#define printf_err tfp_error_printf
#define panic tfp_panic
#define printf tfp_printf
#define sprintf tfp_sprintf

#endif
