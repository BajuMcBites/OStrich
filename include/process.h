#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "event.h"
#include "vm.h"
#include "printf.h"
#include "atomic.h"

#define NR_TASKS 32

extern struct PCB* task[NR_TASKS];
extern int curr_task;


enum sig_val {
    SIGHUP=1,
    SIGINT=2,
    SIGQUIT=3,
    SIGILL=4,
    SIGTRAP=5,
    SIGABRT=6,
    SIGBUS=7,
    SIGFPE=8,
    SIGKILL=9,
    SIGUSR1=10,
    SIGSEGV=11,
    SIGUSR2=12,
    SIGPIPE=13,
    SIGALRM=14,
    SIGTERM=15,
    SIGSTKFLT=16,
    SIGCHLD=17,
    SIGCONT=18,
    SIGSTOP=19,
    SIGTSTP=20,
    SIGTTIN=21,
    SIGTTOU=22,
    SIGURG=23,
    SIGXCPU=24,
    SIGXFSZ=25,
    SIGVTALRM=26,
    SIGPROF=27,
    SIGWINCH=28,
    SIGIO=29,
    SIGPWR=30,
    SIGSYS=31
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
    LockedQueue<Signal, SpinLock> sigs;
    Semaphore* waiting_parent;
    PCB* parent;
    PCB* child_start;
    PCB* child_end;
    PCB* next;
    PCB* before;

    PCB() {
        while (task[curr_task] != nullptr) {
            curr_task = (curr_task + 1) % NR_TASKS;
        }
        task[curr_task] = this;
        pid = curr_task;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
        waiting_parent = nullptr;
    }
    PCB(int id) {
        task[pid] = this;
        pid = id;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
        waiting_parent = nullptr;
        child_start = child_end = next = nullptr;
    }
    
    void raise_signal(Signal* s) {
        sigs.add(s);
    }

    void add_waiting_parent(Semaphore* s, PCB* p) {
        waiting_parent = s;
        parent = p;
    }

    void add_child(PCB* child) {
        if (child_start == nullptr) {
            child_start = child_end = child;
        } else {
            child_end->next = child;
            child->before = child_end;
        }
    }

    void remove_child(PCB* child) {
        child->before = child->next;
    }

    ~PCB() {
        delete page_table;
        delete supp_page_table;
    }
};

void fork(struct UserTCB* tcb);

#endif /*_PROCESS_H_*/