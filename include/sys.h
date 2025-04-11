#ifndef _SYS__H_
#define _SYS___H_

extern "C" void syscall_handler(void* syscall_frame);

void invoke_handler(int opcode);

#endif