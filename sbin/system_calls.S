#include "system_calls.h"


.global _exit
_exit:
    mov x8, #SYS_EXIT
    svc #0
    ret

.global _close
_close:
    mov x8, #SYS_CLOSE
    svc #0
    ret

.global _execve
_execve:
    mov x8, #SYS_EXEC
    svc #0
    ret


.global _fork
_fork:
    mov x8, #SYS_FORK
    svc #0
    ret

.global _fstat
_fstat:
    mov x8, #SYS_FSTAT
    svc #0
    ret


.global _getpid
_getpid:
    mov x8, #SYS_GETPID
    svc #0
    ret


.global _isatty
_isatty:
    mov x8, #SYS_ISATTY
    svc #0
    ret

.global _kill
_kill:
    mov x8, #SYS_KILL
    svc #0
    ret

.global _link
_link:
    mov x8, #SYS_LINK
    svc #0
    ret

.global _lseek
_lseek:
    mov x8, #SYS_LSEEK
    svc #0
    ret

.global _open
_open:
    mov x8, #SYS_OPEN
    svc #0
    ret

.global _read
_read:
    mov x8, #SYS_READ
    svc #0
    ret

.global _stat
_stat:
    mov x8, #SYS_STAT
    svc #0
    ret

.global _unlink
_unlink:
    mov x8, #SYS_UNLINK
    svc #0
    ret

.global _wait
_wait:
    mov x8, #SYS_WAIT
    svc #0
    ret

.global _write
_write:
    mov x8, #SYS_WRITE
    svc #0
    ret

.global _gettimeofday
_gettimeofday:
    mov x8, #SYS_TIME
    svc #0
    ret