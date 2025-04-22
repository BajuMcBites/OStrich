#include "sys.h"

#include "../user_programs/system_calls.h"
#include "event.h"
#include "printf.h"
#include "process.h"
#include "stdint.h"
#include "vm.h"
#include "utils.h"
#include "event.h"
#include "elf_loader.h"
#include "atomic.h"
#include "ramfs.h"
#include "mmap.h"
// #include "trap_frame.h"

// SyscallFrame structure.
// X[0] - return value
struct SyscallFrame {
    union {
        /* X[0] ... X[5] - arguments
        once function is called, X[0] will be the return value.
        */
        uint64_t X[31];  // 0 ... 30 
    };
};

// Function prototypes
void newlib_handle_exit(SyscallFrame* frame);
int newlib_handle_close(SyscallFrame* frame);
int newlib_handle_exec(SyscallFrame* frame);
int newlib_handle_fork(SyscallFrame* frame);
int newlib_handle_fstat(SyscallFrame* frame);
int newlib_handle_getpid(SyscallFrame* frame);
int newlib_handle_isatty(SyscallFrame* frame);
int newlib_handle_kill(SyscallFrame* frame);
int newlib_handle_link(SyscallFrame* frame);
int newlib_handle_lseek(SyscallFrame* frame);
int newlib_handle_open(SyscallFrame* frame);
int newlib_handle_read(SyscallFrame* frame);
int newlib_handle_stat(SyscallFrame* frame);
int newlib_handle_unlink(SyscallFrame* frame);
int newlib_handle_wait(SyscallFrame* frame);
int newlib_handle_write(SyscallFrame* frame);
int newlib_handle_time(SyscallFrame* frame);
void handle_newlib_syscall(int opcode, SyscallFrame* frame);

void syscall_handler(void* syscall_frame) {
    auto* frame = (SyscallFrame*)(syscall_frame);
    int opcode = frame->X[8];

    // Right now, calling any variant of "printf" breaks the kernel causing an infinite exception
    // loop.
    handle_newlib_syscall(opcode, frame);
    // handle_linux_syscall(opcode, frame);
}

void handle_newlib_syscall(int opcode, SyscallFrame* frame) {
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
        case 18:
            printf("Parent done\n");
            break;
        case 19:
            printf("Child done\n");
            break;
        default:
            break;
    }
}

void handle_linux_syscall(int opcode, SyscallFrame* frame) {
    // TODO: Implement Linux syscalls.
    switch (opcode) {
        default:
            break;
    }
}

// Parse arguments from the frame and call the appropriate function.
// frame[0] ... frame[5] would be where the first 6 arguments are.
// See https://sourceware.org/newlib/libc.html#Stubs for the specific arguments for each call.
void newlib_handle_exit(SyscallFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID()); 
    printf("exit has ran, returning %d\n", frame->X[0]);
    if (tcb->pcb->parent != nullptr) {
        Signal* s = new Signal(SIGCHLD, tcb->pcb->pid, frame->X[0]);
        printf("%d this is parent pid\n", tcb->pcb->parent->pid);
        tcb->pcb->parent->raise_signal(s);
        if (tcb->pcb->waiting_parent) {
            tcb->pcb->waiting_parent->up();
        }
    }
    tcb->state = TASK_KILLED;
    event_loop();
}

int newlib_handle_close(SyscallFrame* frame) {
    // TODO: Implement close.
    return 0;
}

int newlib_handle_exec(SyscallFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID()); 
    tcb->state = TASK_STOPPED;
    PCB* pcb = tcb->pcb;
    int pid = pcb->pid;
    printf("calling exec with pathname at %x%x, with pid %d\n", frame->X[0] >> 32, frame->X[0], pid);
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
    
    
    uint64_t sp = 0x0000fffffffff000;
    sema->down([=]() {
        mmap(pcb, sp - PAGE_SIZE, PF_R | PF_W, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr,
             0, PAGE_SIZE, [=]() {
                 load_mmapped_page(pcb, sp - PAGE_SIZE, [=](uint64_t kvaddr) {
                    printf("this is sskvaddr. %x%x\n", kvaddr >> 32, kvaddr);
                     pcb->page_table->use_page_table();
                     sema->up();
                 });
             });
    });
    // set up stack
    sema->down([=]() {
        pcb->page_table->use_page_table();
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
        sema->up();
    });
    
    sema->down([=]() {
        printf("we queued it %d\n", pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        kfree(buffer);
    });
    event_loop();
    return 0;
}

int newlib_handle_fork(SyscallFrame* frame) {
    printf("called fork\n");
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, frame->X);
    fork(tcb);
    event_loop();
    return 0;
}

int newlib_handle_fstat(SyscallFrame* frame) {
    // TODO: Implement fstat.
    return 0;
}

int newlib_handle_getpid(SyscallFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID()); 
    return tcb->pcb->pid;
}

int newlib_handle_isatty(SyscallFrame* frame) {
    // TODO: Implement isatty.
    return 0;
}

int newlib_handle_kill(SyscallFrame* frame) {
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

int newlib_handle_link(SyscallFrame* frame) {
    // TODO: Implement link.
    return 0;
}

int newlib_handle_lseek(SyscallFrame* frame) {
    // TODO: Implement lseek.
    return 0;
}

int newlib_handle_open(SyscallFrame* frame) {
    // TODO: Implement open.
    return 0;
}

int newlib_handle_read(SyscallFrame* frame) {
    // TODO: Implement read.
    return 0;
}

int newlib_handle_stat(SyscallFrame* frame) {
    // TODO: Implement stat.
    return 0;
}

int newlib_handle_unlink(SyscallFrame* frame) {
    // TODO: Implement unlink.
    return 0;
}

int newlib_handle_wait(SyscallFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID()); 
    save_user_context(tcb, frame->X);
    tcb->state = TASK_STOPPED;
    PCB* cur = tcb->pcb;
    int* status_location = (int*)frame->X[0];
    Signal* sig;
    int n_pid = -1;
    bool terminated = false;
    printf("this is cur addy %x%x\n", (uint64_t)tcb->pcb->sigs >> 32, tcb->pcb->sigs);
    while (true) {
        sig = tcb->pcb->sigs->remove();
        if (sig == nullptr) break;
        if (sig->val == SIGKILL) {
            terminated = true;
            break;
        }
        if (sig->val == SIGCHLD) {
            printf("we have a sigchlld lmoa\n");
            cur->page_table->use_page_table();
            *status_location = sig->status;
            n_pid = sig->from_pid;
            break;
        }
    }
    int new_pc = (int)frame->X[30];
    if (terminated) {
        tcb->state = TASK_KILLED;
        event_loop();
    } else if (n_pid != -1) {
        // child already terminated
        set_return_value(tcb, n_pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        event_loop();
    }
    // need to wait for child to terminate
    Semaphore* sema = new Semaphore();
    for (PCB* start = cur->child_start; start != nullptr; start = start->next) {
        printf("added parent sema\n");
        start->add_waiting_parent(sema);
    }
    sema->down([=]{
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
        // set_return_value(tcb, n_pid);
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
    });
    event_loop();
    return 0;
}

int newlib_handle_write(SyscallFrame* frame) {
    // TODO: Implement write.
    return 0;
}

int newlib_handle_time(SyscallFrame* frame) {
    // TODO: Implement time.
    return 0;
}
