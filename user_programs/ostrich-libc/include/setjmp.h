/*
        setjmp.h
        stubs for future use.
*/

#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <machine/setjmp.h>

#include "_ansi.h"

_BEGIN_STD_C

#ifdef __GNUC__
void longjmp(jmp_buf __jmpb, int __retval) __attribute__((__noreturn__));
#else
void longjmp(jmp_buf __jmpb, int __retval);
#endif
int setjmp(jmp_buf __jmpb);

_END_STD_C

#endif /* _SETJMP_H_ */
