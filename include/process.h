#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "vm.h"
#include "printf.h"

#define NR_TASKS 256

extern struct PCB* task[NR_TASKS];
extern int curr_task;

struct PCB {
    int pid;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;

    PCB() {
        while (task[curr_task] != nullptr) {
            curr_task = (curr_task + 1) % NR_TASKS;
        }
        task[curr_task] = this;
        pid = curr_task;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
    }
    PCB(int id) {
        task[pid] = this;
        pid = id;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
    }

    ~PCB() {
        delete page_table;
        delete supp_page_table;
    }
};

#endif /*_PROCESS_H_*/