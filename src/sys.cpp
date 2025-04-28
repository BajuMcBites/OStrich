#include "sys.h"

#include "../filesystem/filesys/fs_requests.h"
#include "../user_programs/system_calls.h"
#include "atomic.h"
#include "elf_loader.h"
#include "event.h"
#include "file_table.h"
#include "framebuffer.h"
#include "fs.h"
#include "mmap.h"
#include "printf.h"
#include "process.h"
#include "ramfs.h"
#include "stdint.h"
#include "tty.h"
#include "utils.h"
#include "vm.h"
// #include "trap_frame.h"

// Function prototypes
void newlib_handle_exit(KernelEntryFrame* frame);
void newlib_handle_close(KernelEntryFrame* frame);
int newlib_handle_exec(KernelEntryFrame* frame);
int newlib_handle_fork(KernelEntryFrame* frame);
int newlib_handle_fstat(KernelEntryFrame* frame);
int newlib_handle_getpid(KernelEntryFrame* frame);
int newlib_handle_isatty(KernelEntryFrame* frame);
int newlib_handle_kill(KernelEntryFrame* frame);
int newlib_handle_link(KernelEntryFrame* frame);
int newlib_handle_lseek(KernelEntryFrame* frame);
void newlib_handle_open(KernelEntryFrame* frame);
void newlib_handle_read(KernelEntryFrame* frame);
int newlib_handle_stat(KernelEntryFrame* frame);
int newlib_handle_unlink(KernelEntryFrame* frame);
int newlib_handle_wait(KernelEntryFrame* frame);
void newlib_handle_write(KernelEntryFrame* frame);
int newlib_handle_time(KernelEntryFrame* frame);
int newlib_handle_sbrk(KernelEntryFrame* frame);
int newlib_handle_mmap(KernelEntryFrame* frame);

void handle_newlib_syscall(int opcode, KernelEntryFrame* frame);

void set_return_value_and_state(UserTCB* tcb, int value, int state) {
    set_return_value(tcb, value);
    tcb->state = state;
    queue_user_tcb(tcb);
}

void handle_success(UserTCB* tcb, int value) {
    set_return_value_and_state(tcb, value, TASK_RUNNING);
}

void handle_error(UserTCB* tcb, Semaphore* sema) {
    set_return_value_and_state(tcb, -1, TASK_RUNNING);
    if (sema != nullptr) {
        sema->kill();
        delete sema;
    }
}

int sys_draw_frame(KernelEntryFrame* frame);

void syscall_handler(KernelEntryFrame* frame) {
    // printf("hello\n");
    int opcode = frame->X[8];
    if (opcode == DRAW_FRAME) {
        frame->X[0] = sys_draw_frame(frame);
        return;
    }
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
            printf("SYS_CLOSE\n");
            newlib_handle_close(frame);
            K::assert(false, "RACE DETECTED IN CLOSE");
            break;
        case NEWLIB_EXEC:
            frame->X[0] = newlib_handle_exec(frame);
            K::assert(false, "RACE DETECTED IN EXEC");
            break;
        case NEWLIB_FORK:
            frame->X[0] = newlib_handle_fork(frame);
            K::assert(false, "RACE DETECTED IN FORK");
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
            newlib_handle_lseek(frame);
            break;
        case NEWLIB_OPEN:
            printf("SYS_OPEN\n");
            newlib_handle_open(frame);
            K::assert(false, "RACE DETECTED IN OPEN");
            break;
        case NEWLIB_READ:
            printf("SYS_READ\n");
            newlib_handle_read(frame);
            K::assert(false, "RACE DETECTED IN READ");
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
            printf("SYS_WRITE\n");
            newlib_handle_write(frame);
            K::assert(false, "RACE DETECTED IN WRITE");
            break;
        case NEWLIB_TIME:
            frame->X[0] = newlib_handle_time(frame);
            break;
        case NEWLIB_SBRK:
            newlib_handle_sbrk(frame);
            break;
        case NEWLIB_MMAP:
            newlib_handle_mmap(frame);
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

void newlib_handle_close(KernelEntryFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;
    FileTable* file_table = pcb->file_table;
    save_user_context(tcb, frame);

    int fd = frame->X[0];
    file_table->remove_file(fd, [tcb](int fd) {
        return fd < 0 ? handle_error(tcb, nullptr) : handle_success(tcb, 0 /* success */);
    });
    event_loop();
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
    sp -= sp % 16;
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

void newlib_handle_open(KernelEntryFrame* frame) {
    char* path_name = (char*)frame->X[0];
    int flags = frame->X[1];
    int mode = frame->X[2];

    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;
    FileTable* file_table = pcb->file_table;
    save_user_context(tcb, frame);

    int cwd = pcb->cwd;
    bool is_dir = flags & OpenFlags::O_DIRECTORY;
    bool is_append = flags & OpenFlags::O_APPEND;
    bool is_create = flags & OpenFlags::O_CREAT;
    bool is_trunc = flags & OpenFlags::O_TRUNC;
    uint16_t permissions = mode;

    // Handle the create case first.
    Semaphore* sema = new Semaphore(1);
    if (is_create) {
        sema->down([=]() {
            fs::issue_fs_create_file(
                cwd, is_dir, path_name, permissions, [tcb, sema](fs::fs_response_t resp) {
                    if (resp.data.create_file.status != fs::fs_resp_status_t::FS_RESP_SUCCESS) {
                        printf("Failed to create file\n");
                        handle_error(tcb, sema);
                        return;
                    }
                    sema->up();
                });
        });
    }

    // Once it's created, open it in the kernel and return an fd.
    sema->down([=]() {
        kopen(path_name, [=](KFile* file) {
            if (file == nullptr) {
                handle_error(tcb, sema);
                return;
            }

            file_table->add_file(file, 0, flags, [=](int fd) {
                return fd <= 0 ? handle_error(tcb, sema) : handle_success(tcb, fd);
            });
            sema->up();
        });
    });
    event_loop();
}

void newlib_handle_read_or_write(KernelEntryFrame* frame, bool is_read) {
    printf("newlib_handle_read_or_write: starting\n");
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;
    FileTable* file_table = pcb->file_table;
    save_user_context(tcb, frame);

    int fd = frame->X[0];
    char* buf = (char*)frame->X[1];
    int count = frame->X[2];

    // Per-process file pointer (1:1 with file descriptors).
    printf("newlib_handle_read_or_write: fd = %d, core = %d, is_read = %d\n", fd, getCoreID(),
           is_read);
    UFile& ufile = file_table->get_file(fd);
    KFile* file = ufile.backing_file();

    if (file == nullptr) {
        handle_error(tcb, nullptr);
        return;
    }
    printf("newlib_handle_read_or_write: file = %x\n", file);
    printf("newlib_handle_read_or_write: file->get_inode_number() = %d\n",
           file->get_inode_number());

    // Don't write to read-only file..
    if (!is_read && ufile.mode_flags() & OpenFlags::O_RDONLY) {
        printf("newlib_handle_read_or_write: file is read-only\n");
        handle_error(tcb, nullptr);
        return;
    }

    // Don't read from write-only file.
    if (is_read && ufile.mode_flags() & OpenFlags::O_WRONLY) {
        printf("newlib_handle_read_or_write: file is write-only\n");
        handle_error(tcb, nullptr);  // calls event_loop()
        return;
    }

    // Capture the current offset to avoid race conditions
    uint64_t current_offset = ufile.offset();
    printf("newlib_handle_read_or_write: current_offset = %d\n", current_offset);

    // Perform the read or write.
    auto handle_return = [tcb, fd, file_table,
                          current_offset](uint64_t bytes_read_or_write) mutable {
        UFile& ufile = file_table->get_file(fd);
        // Set the new offset based on the original offset plus bytes processed
        printf("handle_return: bytes_read_or_write = %d, new_offset = %d, old_offset = %d\n",
               bytes_read_or_write, current_offset + bytes_read_or_write, ufile.offset());
        ufile.set_offset(current_offset + bytes_read_or_write);
        return bytes_read_or_write < 0 ? handle_error(tcb, nullptr)
                                       : handle_success(tcb, bytes_read_or_write);
    };

    if (is_read) {
        kread(file, current_offset, buf, count, handle_return);
    } else {
        kwrite(file, current_offset, buf, count, handle_return);
    }
    event_loop();
}

void newlib_handle_read(KernelEntryFrame* frame) {
    newlib_handle_read_or_write(frame, true);
    event_loop();  // safety
}

void newlib_handle_write(KernelEntryFrame* frame) {
    newlib_handle_read_or_write(frame, false);
    event_loop();  // safety
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

int newlib_handle_time(KernelEntryFrame* frame) {
    // TODO: Implement time.
    return 0;
}

int newlib_handle_sbrk(KernelEntryFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    printf("called sbrk with %d\n", frame->X[0]);
    uint64_t ret_address = tcb->pcb->data_end;
    tcb->pcb->data_end += frame->X[0];
    frame->X[0] = ret_address;
    printf("ret address is 0x%X%X\n", frame->X[0] >> 32, frame->X[0]);
    return 0;
}

int newlib_handle_mmap(KernelEntryFrame* frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;
    save_user_context(tcb, frame);

    uint64_t length = frame->X[1];
    uint64_t prot = frame->X[2];
    uint64_t flags = frame->X[3];
    int fd = frame->X[4];
    uint64_t offset = frame->X[5];

    uint64_t data_end = pcb->data_end;

    uint64_t first_user_addr = data_end + (PAGE_SIZE - (data_end % PAGE_SIZE));

    data_end = first_user_addr + ((length / PAGE_SIZE) * PAGE_SIZE);
    if (length % PAGE_SIZE != 0) {
        data_end += PAGE_SIZE;
    }

    uint64_t ret_address = first_user_addr;

    KFile* file = nullptr;
    if (fd != 0) {
        file = pcb->file_table->get_file(fd).backing_file();
    }
    mmap(pcb, first_user_addr, prot, flags, file, offset, length, [=]() {
        tcb->context.x0 = first_user_addr;
        queue_user_tcb(tcb);
    });
    event_loop();
}

int sys_draw_frame(KernelEntryFrame* frame) {
    // Retrieve the pointer to the frame buffer data from the first argument
    void* frame_data = (void*)frame->X[0];

    // Access the current task's PCB to get the framebuffer buffer
    Framebuffer* fb = fb_get();
    if (fb == nullptr) {
        UserTCB* tcb = get_running_user_tcb(getCoreID());
        PCB* pcb = tcb->pcb;
        if (pcb->frameBuffer == nullptr) {
            pcb->frameBuffer = request_tty();
        }
        tcb->frameBuffer = pcb->frameBuffer;
    }

    fb = fb_get();
    K::assert(fb != nullptr, "fb null\n");

    // Perform the memory copy operation
    K::memcpy(fb->buffer, frame_data, 1228800);  // Frame buffer size

    return 0;  // Return success
}