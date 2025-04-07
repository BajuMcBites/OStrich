#ifndef _SYS__H_
#define _SYS___H_

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

extern "C" void syscall_handler(void* syscall_frame);

#endif