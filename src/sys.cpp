#include "sys.h"

#include "../user_programs/system_calls.h"
#include "atomic.h"
#include "elf_loader.h"
#include "event.h"
#include "mmap.h"
#include "printf.h"
#include "process.h"
#include "ramfs.h"
#include "stdint.h"
#include "utils.h"
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
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    printf("exit has ran, returning %d\n", frame->X[0]);
    if (tcb->pcb->parent != nullptr) {
        // throw signals at parent processes
        Signal* s = new Signal(SIGCHLD, tcb->pcb->pid, frame->X[0]);
        tcb->pcb->parent->remove_child(tcb->pcb);
        tcb->pcb->parent->raise_signal(s);

        if (tcb->pcb->waiting_parent) {
            printf("sema is not null for %d\n", tcb->pcb->pid);
            tcb->pcb->waiting_parent->up();
        } else {
            printf("sema is null for %d\n", tcb->pcb->pid);
        }
    }
    // orphan the child processes
    for (PCB* start = tcb->pcb->child_start; start != nullptr; start = start->next) {
        start->parent = nullptr;
    }
    delete tcb;
    event_loop();
}

int newlib_handle_close(KernelEntryFrame* frame) {
    // TODO: Implement close.
    return 0;
}

int newlib_handle_exec(KernelEntryFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    tcb->state = TASK_STOPPED;
    PCB* pcb = tcb->pcb;
    pcb->supp_page_table = new SupplementalPageTable();
    pcb->page_table = new PageTable();
    int pid = pcb->pid;
    // printf("calling exec with pathname at %x%x, with pid %d\n", frame->X[0] >> 32, frame->X[0],
    // pid);
    char* pathname = (char*)frame->X[0];
    char** argv = (char**)frame->X[1];
    int elf_index = get_ramfs_index(pathname);
    if (elf_index == -1) {
        printf("invalid file name!\n");
        return 1;
    }
    // calculate argc
    int argc = 0;
    for (; *argv[argc] != 0; argc++);
    // // load elf file
    const int sz = ramfs_size(elf_index);
    char* buffer = (char*)kmalloc(sz);
    ramfs_read(buffer, 0, sz, elf_index);
    Semaphore* sema = new Semaphore(1);
    void* new_pc = elf_load((void*)buffer, pcb, sema);

    // set up stack
    uint64_t sp = 0x0000fffffffff000;
    uint64_t addrs[argc];
    for (int i = argc - 1; i >= 0; --i) {
        int len = K::strlen(argv[i]) + 1;
        sp -= len;
        addrs[i] = sp;
        K::memcpy((void*)sp, argv[i], len);
    }
    sp &= ~((uint64_t)7);
    sp -= 8;
    *(uint64_t*)sp = 0;
    for (int i = argc - 1; i >= 0; --i) {
        sp -= 8;
        *(uint64_t*)sp = addrs[i];
    }
    // save &argv
    tcb->context.x1 = sp;
    // save argc
    tcb->context.x0 = argc;
    tcb->context.sp = sp;
    tcb->context.pc = (uint64_t)new_pc;
    tcb->context.x30 = (uint64_t)new_pc; /* this just to repeat the user prog again and again*/

    sema->down([=]() {
        // printf("we queued it %d\n", pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        kfree(buffer);
    });
    event_loop();
    return 0;
}

int newlib_handle_fork(KernelEntryFrame* frame) {
    // printf("called fork\n");
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, frame);
    fork(tcb);
    event_loop();
    return 0;
}

int newlib_handle_fstat(KernelEntryFrame* frame) {
    // TODO: Implement fstat.
    return 0;
}

int newlib_handle_getpid(KernelEntryFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    return tcb->pcb->pid;
}

int newlib_handle_isatty(KernelEntryFrame* frame) {
    // TODO: Implement isatty.
    return 0;
}

int newlib_handle_kill(KernelEntryFrame* frame) {
    int pid = frame->X[0];
    int sig = frame->X[1];
    if (pid < 1) {
        printf("dont have process groups implemented\n");
        return 1;
    } else {
        UserTCB* tcb = get_running_user_tcb(getCoreID());
        int curr_pid = tcb->pcb->pid;
        PCB* target = task[pid];
        if (target == nullptr) {
            printf("invalid pid\n");
            return 1;
        }
        Signal* s = new Signal(sig, curr_pid, 0);
        target->sigs->add(s);
    }
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
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, frame);
    tcb->state = TASK_STOPPED;
    PCB* cur = tcb->pcb;
    int* status_location = (int*)frame->X[0];
    Signal* sig;
    int n_pid = -1;
    bool terminated = false;
    // check among existing signals if it already has an exited child or if this is a terminated
    // process
    while (true) {
        sig = tcb->pcb->sigs->remove();
        if (sig == nullptr) break;
        if (sig->val == SIGKILL) {
            terminated = true;
            break;
        }
        if (sig->val == SIGCHLD) {
            cur->page_table->use_page_table();
            *status_location = sig->status;
            n_pid = sig->from_pid;
            break;
        }
    }
    if (terminated) {
        tcb->state = TASK_KILLED;
        event_loop();
    } else if (n_pid != -1) {
        // child already terminated
        set_return_value(tcb, n_pid);
        printf("wait returned, returning %d\n", n_pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        event_loop();
    }
    // otherwise we need to wait for child to terminate
    Semaphore* sema = new Semaphore();
    for (PCB* start = cur->child_start; start != nullptr; start = start->next) {
        printf("added parent sema to pid %d\n", start->pid);
        if (start->waiting_parent) delete start->waiting_parent;
        start->add_waiting_parent(sema);
    }
    sema->down([=] {
        // among child exit, sema will be unlocked
        Signal* sig;
        int n_pid = -1;
        bool terminated = false;
        while (sig = (tcb->pcb->sigs->remove())) {
            if (sig->val == SIGKILL) {
                terminated = true;
                break;
            }
            if (sig->val == SIGCHLD) {
                cur->page_table->use_page_table();
                *status_location = sig->status;
                n_pid = sig->from_pid;
                break;
            }
        }
        if (n_pid == -1) {
            printf("no SIGCHLD signal - something's wrong\n");
        }

        set_return_value(tcb, n_pid);
        printf("wait returned, returning %d\n", n_pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
    });
    event_loop();
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
