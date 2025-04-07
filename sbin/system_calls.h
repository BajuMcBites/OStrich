#ifndef _SYS_H_
#define _SYS_H_

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
extern int close(int file);
extern char *
    *environ; /* pointer to array of char * strings that define the current environment variables */
extern int execve(char *name, char **argv, char **env);
extern int fork();
extern int fstat(int file, struct stat *st);
extern int getpid();
extern int isatty(int file);
extern int kill(int pid, int sig);
extern int link(char *old, char *new);
extern int lseek(int file, int ptr, int dir);
extern int open(const char *name, int flags, ...);
extern int read(int file, char *ptr, int len);
// extern caddr_t sbrk(int incr);
extern int stat(const char *file, struct stat *st);
// extern clock_t times(struct tms *buf);
extern int unlink(char *name);
extern int wait(int *status);
extern int write(int file, char *ptr, int len);
extern int gettimeofday(struct timeval *p, struct timezone *z);

#endif
