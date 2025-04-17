#include "sys.h"

#include "../user_programs/system_calls.h"
#include "printf.h"
#include "stdint.h"
#include "vm.h"
// #include "trap_frame.h"

// Function prototypes
void newlib_handle_exit(KernelEntryFrame* frame);
int newlib_handle_close(KernelEntryFrame* frame);
int newlib_handle_exec(KernelEntryFrame* frame);
int newlib_handle_fork(KernelEntryFrame* frame);
int newlib_handle_fstat(KernelEntryFrame* frame);
int newlib_handle_getpid(KernelEntryFrame* frame);
int newlib_handle_isatty(KernelEntryFrame* frame);
int newlib_handle_kill(KernelEntryFrame* frame);
int newlib_handle_link(KernelEntryFrame* frame);
int newlib_handle_lseek(KernelEntryFrame* frame);
int newlib_handle_open(KernelEntryFrame* frame);
int newlib_handle_read(KernelEntryFrame* frame);
int newlib_handle_stat(KernelEntryFrame* frame);
int newlib_handle_unlink(KernelEntryFrame* frame);
int newlib_handle_wait(KernelEntryFrame* frame);
int newlib_handle_write(KernelEntryFrame* frame);
int newlib_handle_time(KernelEntryFrame* frame);
void handle_newlib_syscall(int opcode, KernelEntryFrame* frame);

void syscall_handler(KernelEntryFrame* frame) {
    int opcode = frame->X[8];

    // Right now, calling any variant of "printf" breaks the kernel causing an infinite exception
    // loop.
    handle_newlib_syscall(opcode, frame);
    // handle_linux_syscall(opcode, frame);
}

void handle_newlib_syscall(int opcode, KernelEntryFrame* frame) {
    switch (opcode) {
        case NEWLIB_EXIT:
            newlib_handle_exit(frame);
            break;
        case NEWLIB_CLOSE:
            frame->X[0] = newlib_handle_close(frame);
            break;
        case NEWLIB_EXEC:
            frame->X[0] = newlib_handle_exec(frame);
            break;
        case NEWLIB_FORK:
            frame->X[0] = newlib_handle_fork(frame);
            break;
        case NEWLIB_FSTAT:
            frame->X[0] = newlib_handle_fstat(frame);
            break;
        case NEWLIB_GETPID:
            frame->X[0] = newlib_handle_getpid(frame);
            break;
        case NEWLIB_ISATTY:
            frame->X[0] = newlib_handle_isatty(frame);
            break;
        case NEWLIB_KILL:
            frame->X[0] = newlib_handle_kill(frame);
            break;
        case NEWLIB_LINK:
            frame->X[0] = newlib_handle_link(frame);
            break;
        case NEWLIB_LSEEK:
            frame->X[0] = newlib_handle_lseek(frame);
            break;
        case NEWLIB_OPEN:
            frame->X[0] = newlib_handle_open(frame);
            break;
        case NEWLIB_READ:
            frame->X[0] = newlib_handle_read(frame);
            break;
        case NEWLIB_STAT:
            frame->X[0] = newlib_handle_stat(frame);
            break;
        case NEWLIB_UNLINK:
            frame->X[0] = newlib_handle_unlink(frame);
            break;
        case NEWLIB_WAIT:
            frame->X[0] = newlib_handle_wait(frame);
            break;
        case NEWLIB_WRITE:
            frame->X[0] = newlib_handle_write(frame);
            break;
        case NEWLIB_TIME:
            frame->X[0] = newlib_handle_time(frame);
            break;
        default:
            break;
    }
}

void handle_linux_syscall(int opcode, KernelEntryFrame* frame) {
    // TODO: Implement Linux syscalls.
    switch (opcode) {
        default:
            break;
    }
}

// Parse arguments from the frame and call the appropriate function.
// frame[0] ... frame[5] would be where the first 6 arguments are.
// See https://sourceware.org/newlib/libc.html#Stubs for the specific arguments for each call.
void newlib_handle_exit(KernelEntryFrame* frame) {
    // TODO: Implement exit.
}

int newlib_handle_close(KernelEntryFrame* frame) {
    // TODO: Implement close.
    return 0;
}

int newlib_handle_exec(KernelEntryFrame* frame) {
    // TODO: Implement exec.
    return 0;
}

int newlib_handle_fork(KernelEntryFrame* frame) {
    // TODO: Implement fork.
    return 0;
}

int newlib_handle_fstat(KernelEntryFrame* frame) {
    // TODO: Implement fstat.
    return 0;
}

int newlib_handle_getpid(KernelEntryFrame* frame) {
    // TODO: Implement getpid.
    return 0;
}

int newlib_handle_isatty(KernelEntryFrame* frame) {
    // TODO: Implement isatty.
    return 0;
}

int newlib_handle_kill(KernelEntryFrame* frame) {
    // TODO: Implement kill.
    return 0;
}

int newlib_handle_link(KernelEntryFrame* frame) {
    // TODO: Implement link.
    return 0;
}

int newlib_handle_lseek(KernelEntryFrame* frame) {
    // TODO: Implement lseek.
    return 0;
}

int newlib_handle_open(KernelEntryFrame* frame) {
    // TODO: Implement open.
    return 0;
}

int newlib_handle_read(KernelEntryFrame* frame) {
    // TODO: Implement read.
    return 0;
}

int newlib_handle_stat(KernelEntryFrame* frame) {
    // TODO: Implement stat.
    return 0;
}

int newlib_handle_unlink(KernelEntryFrame* frame) {
    // TODO: Implement unlink.
    return 0;
}

int newlib_handle_wait(KernelEntryFrame* frame) {
    // TODO: Implement wait.
    return 0;
}

int newlib_handle_write(KernelEntryFrame* frame) {
    // TODO: Implement write.
    return 0;
}

int newlib_handle_time(KernelEntryFrame* frame) {
    // TODO: Implement time.
    return 0;
}
