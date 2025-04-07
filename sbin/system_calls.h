#ifndef _SYS__CALL_H_
#define _SYS__CALL_H_

#include "stdint.h"

#define SYS_EXIT 0
#define SYS_CLOSE 1
#define SYS_ENV 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_FSTAT 5
#define SYS_GETPID 6
#define SYS_ISATTY 7
#define SYS_KILL 8
#define SYS_LINK 9
#define SYS_LSEEK 10
#define SYS_OPEN 11
#define SYS_READ 12
#define SYS_STAT 13
#define SYS_UNLINK 14
#define SYS_WAIT 15
#define SYS_WRITE 16
#define SYS_TIME 17

extern void _exit();
extern int _close(int file);
extern char *
    *environ; /* pointer to array of char * strings that define the current environment variables */
extern int _execve(char *name, char **argv, char **env);
extern int _fork();
extern int _fstat(int file, struct stat *st);
extern int _getpid();
extern int _isatty(int file);
extern int _kill(int pid, int sig);
extern int _link(char *old, char *new);
extern int _lseek(int file, int ptr, int dir);
extern int _open(const char *name, int flags, ...);
extern int _read(int file, char *ptr, int len);
// extern caddr_t _sbrk(int incr);
extern int _stat(const char *file, struct stat *st);
// extern clock_t times(struct tms *buf);
extern int _unlink(char *name);
extern int _wait(int *status);
extern int _write(int file, char *ptr, int len);
extern int _gettimeofday(struct timeval *p, struct timezone *z);

#endif
