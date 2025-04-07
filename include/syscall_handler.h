#ifndef __SYSCALL_HANDLER_H
#define __SYSCALL_HANDLER_H

/*
syscall mappings go somewhere.
*/

extern "C" {
void invoke_handler(int opcode);
void svc_handler(void);
static inline unsigned int get_current_el(void);
}

#endif