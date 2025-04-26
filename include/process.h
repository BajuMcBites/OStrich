#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "atomic.h"
#include "event.h"
#include "printf.h"
#include "vm.h"

#define NR_TASKS (1 << 8)

extern struct PCB* task[NR_TASKS];
extern int curr_task;
extern int task_cnt;

enum sig_val {
    SIGHUP = 1,
    SIGINT = 2,
    SIGQUIT = 3,
    SIGILL = 4,
    SIGTRAP = 5,
    SIGABRT = 6,
    SIGBUS = 7,
    SIGFPE = 8,
    SIGKILL = 9,
    SIGUSR1 = 10,
    SIGSEGV = 11,
    SIGUSR2 = 12,
    SIGPIPE = 13,
    SIGALRM = 14,
    SIGTERM = 15,
    SIGSTKFLT = 16,
    SIGCHLD = 17,
    SIGCONT = 18,
    SIGSTOP = 19,
    SIGTSTP = 20,
    SIGTTIN = 21,
    SIGTTOU = 22,
    SIGURG = 23,
    SIGXCPU = 24,
    SIGXFSZ = 25,
    SIGVTALRM = 26,
    SIGPROF = 27,
    SIGWINCH = 28,
    SIGIO = 29,
    SIGPWR = 30,
    SIGSYS = 31
};

struct Signal {
    int val;
    int from_pid;
    int status;
    Signal* next;
    Signal(int id, int from, int stat) {
        val = id;
        from_pid = from;
        status = stat;
    }
};

struct PCB {
    int pid;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;
    LockedQueue<Signal, SpinLock>* sigs;
    Semaphore* waiting_parent;
    PCB* parent;
    // fields for maintaining double linked list of children
    PCB* child_start;
    PCB* child_end;
    PCB* next;
    PCB* before;

    PCB() {
        K::assert(task_cnt < NR_TASKS, "we are out of task space!\n");
        while (task[curr_task] != nullptr) {
            curr_task = (curr_task + 1) % NR_TASKS;
        }
        task[curr_task] = this;
        task_cnt += 1;
        pid = curr_task;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
        waiting_parent = nullptr;
        parent = nullptr;
        child_start = child_end = next = before = nullptr;
        sigs = new LockedQueue<Signal, SpinLock>;
    }
    PCB(int id) {
        if (task[pid])
            delete task[pid];
        task_cnt++;
        pid = id;
        task[pid] = this;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
        waiting_parent = nullptr;
        child_start = child_end = next = nullptr;
        parent = nullptr;
        sigs = new LockedQueue<Signal, SpinLock>;
        before = nullptr;
    }

    void raise_signal(Signal* s) {
        sigs->add(s);
    }

    void add_waiting_parent(Semaphore* s) {
        waiting_parent = s;
    }

    void add_child(PCB* child) {
        child->parent = this;
        if (child_start == nullptr) {
            child_start = child_end = child;
        } else {
            child_end->next = child;
            child->before = child_end;
            child_end = child;
        }
    }

    void remove_child(PCB* child) {
        // linked list logic- if we are removing child_start then the next element is child_start
        // and if we are removing child_end then the element before is child_end
        if (child->before == nullptr) {
            if (child->next == nullptr) {
                child_end = child_start = child->before = child->next = nullptr;
            } else {
                child->next->before = nullptr;
                child_start = child->next;
            }
        } else if (child->next == nullptr) {
            child_end = child->before;
            child->before->next = nullptr;
        } else {
            child->before->next = child->next;
            child->next->before = child->before;
        }
    }

    ~PCB() {
        task_cnt--;
        delete page_table;
        delete supp_page_table;
        delete sigs;
        if (waiting_parent) delete waiting_parent;
    }
};

void fork(struct UserTCB* tcb);

void kill_process(struct PCB* pcb);

#endif /*_PROCESS_H_*/