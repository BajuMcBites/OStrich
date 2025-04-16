#include "sys.h"

#include "../user_programs/system_calls.h"
#include "printf.h"
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
    printf("we are here\n");
    tcb->state = TASK_KILLED;
    event_loop();
}

int newlib_handle_close(SyscallFrame* frame) {
    // TODO: Implement close.
    return 0;
}

int newlib_handle_exec(SyscallFrame* frame) {
    printf("GOT TO EXEC!!\n");
    char* pathname = (char*)frame->X[0];
    char** argv = (char**)frame->X[1];
    int elf_index = get_ramfs_index("user_prog");
    if (elf_index == nullptr) {
        printf("invalid file name!\n");
        return 1;
    }
    UserTCB* tcb = get_running_user_tcb(getCoreID()); 
    PCB* pcb = tcb->pcb;
    const int sz = ramfs_size(elf_index);
    char* buffer = (char*)kmalloc(sz);
    ramfs_read(buffer, 0, sz, elf_index);
    Semaphore* sema = new Semaphore(1);
    void* new_pc = elf_load((void*)buffer, pcb, sema);
    uint64_t sp = 0x0000fffffffff000;
    // only doing this because no eviction
    sema->down([=]() {
        mmap(pcb, sp - PAGE_SIZE, PF_R | PF_W, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr,
             0, PAGE_SIZE, [=]() {
                 load_mmapped_page(pcb, sp - PAGE_SIZE, [=](uint64_t kvaddr) {
                     pcb->page_table->use_page_table();
                     K::memset((void*)(sp - PAGE_SIZE), 0, PAGE_SIZE);
                     sema->up();
                 });
             });
    });
    int argc = 0;
    for (; argv[argc] != "\0"; argc++);
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
        sp -= 8;
        *(uint64_t*)sp = 0;
        for (int i = argc - 1; i >= 0; --i) {
            sp -= 8;
            *(uint64_t*)sp = addrs[i];
        }
        // save &argv
        sp -= 8;
        *(uint64_t*)sp = sp + 8;

        // save argc
        sp -= 8;
        *(uint64_t*)sp = argc;
        tcb->context.sp = sp;
        sema->up();
    });
    tcb->pcb = pcb;
    tcb->context.pc = (uint64_t)new_pc;
    tcb->context.x30 = (uint64_t)new_pc; /* this just to repeat the user prog again and again*/
    printf("%x this is pc\n", tcb->context.pc);
    sema->down([=]() {
        tcb->state = TASK_RUNNING;
    });
    return 0;
}

int newlib_handle_fork(SyscallFrame* frame) {
    printf("GOT TO FORK!!\n");
    UserTCB* curr_tcb = get_running_user_tcb(getCoreID()); 
    UserTCB* new_tcb = new UserTCB();
    K::memcpy((void*)new_tcb, (void*)curr_tcb, sizeof(UserTCB));
    PCB* pcb = new PCB();
    // TODO: make a different copy of the page table that does copy on write?
    pcb->page_table = curr_tcb->pcb->page_table;
    pcb->supp_page_table = curr_tcb->pcb->supp_page_table;
    new_tcb->pcb = pcb;
    set_return_value(new_tcb, 0);
    queue_user_tcb(new_tcb);
    return pcb->pid;
}

int newlib_handle_fstat(SyscallFrame* frame) {
    // TODO: Implement fstat.
    return 0;
}

int newlib_handle_getpid(SyscallFrame* frame) {
    // TODO: Implement getpid.
    return 0;
}

int newlib_handle_isatty(SyscallFrame* frame) {
    // TODO: Implement isatty.
    return 0;
}

int newlib_handle_kill(SyscallFrame* frame) {
    // TODO: Implement kill.
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
    // TODO: Implement wait.
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
