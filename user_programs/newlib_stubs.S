#include "system_calls.h"

/*---------------------------------------------------------
 This file does not do much. This puts the system call type into 
 register x8 similar to how linux does and then generates an 
 interrupt using the svc call. The system call handler can check 
 x8 to identify what system call was called.
 --------------------------------------------------------*/
 
.global _exit
_exit:
    mov x8, #NEWLIB_EXIT
    svc #0
    ret

.global _close
_close:
    mov x8, #NEWLIB_CLOSE
    svc #0
    ret

.global _execve
_execve:
    mov x8, #NEWLIB_EXEC
    svc #0
    ret

.global _fork
_fork:
    mov x8, #NEWLIB_FORK
    svc #0
    ret

.global _fstat
_fstat:
    mov x8, #NEWLIB_FSTAT
    svc #0
    ret

.global _getpid
_getpid:
    mov x8, #NEWLIB_GETPID
    svc #0
    ret


.global _isatty
_isatty:
    mov x8, #NEWLIB_ISATTY
    svc #0
    ret

.global _kill
_kill:
    mov x8, #NEWLIB_KILL
    svc #0
    ret

.global _link
_link:
    mov x8, #NEWLIB_LINK
    svc #0
    ret

.global _lseek
_lseek:
    mov x8, #NEWLIB_LSEEK
    svc #0
    ret

.global _open
_open:
    mov x8, #NEWLIB_OPEN
    svc #0
    ret

.global _read
_read:
    mov x8, #NEWLIB_READ
    svc #0
    ret

.global _stat
_stat:
    mov x8, #NEWLIB_STAT
    svc #0
    ret

.global _unlink
_unlink:
    mov x8, #NEWLIB_UNLINK
    svc #0
    ret

.global _wait
_wait:
    mov x8, #NEWLIB_WAIT
    svc #0
    ret

.global _write
_write:
    mov x8, #NEWLIB_WRITE
    svc #0
    ret

.global _gettimeofday
_gettimeofday:
    mov x8, #NEWLIB_TIME
    svc #0
    ret

// Used during initialization and termination of libc.
// Can call any functions if needed.
.global _init
_init:
    ret

.global _fini
_fini:
    ret

.global _sbrk
_sbrk:
    mov x8, #NEWLIB_SBRK
    svc #0
    ret

.global time_elapsed
time_elapsed:
    mov x8, #NEWLIB_TIME_ELAPSED
    svc #0
    ret
