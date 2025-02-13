#ifndef _FORK_H
#define _FORK_H

extern "C" void ret_from_fork(void);
extern "C" int copy_process(unsigned long fn, unsigned long arg);

#endif