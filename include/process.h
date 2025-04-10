#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "vm.h"

struct PCB {
    int pid;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;

    PCB() {
        pid = -1;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
    }

    ~PCB() {
        delete page_table;
        delete supp_page_table;
    }
};

#endif /*_PROCESS_H_*/