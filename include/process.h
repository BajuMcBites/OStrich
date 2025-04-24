#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "file_table.h"
#include "vm.h"

struct PCB {
    int pid;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;
    FileTable* file_table;

    PCB() {
        pid = -1;
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;
        file_table = new FileTable;
    }

    ~PCB() {
        delete page_table;
        delete supp_page_table;
        delete file_table;
    }
};

#endif /*_PROCESS_H_*/