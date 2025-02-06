#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "irq.h"
#include "timer.h"
#include "core.h"
#include "sched.h"
#include "mm.h"
#include "fork.h"

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__ ((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void) {
    return (uint64_t) &stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void print_ascii_art() {
    printf("\n");
    printf("                                                                                     \n");
    printf("      # ###          #######                                                /       \n");
    printf("    /  /###        /       ###                           #                #/        \n");
    printf("   /  /  ###      /         ##      #                   ###               ##        \n");
    printf("  /  ##   ###     ##        #      ##                    #                ##        \n");
    printf(" /  ###    ###     ###             ##                                     ##        \n");
    printf("##   ##     ##    ## ###         ######## ###  /###    ###        /###    ##  /##   \n");
    printf("##   ##     ##     ### ###      ########   ###/ #### /  ###      / ###  / ## / ###  \n");
    printf("##   ##     ##       ### ###       ##       ##   ###/    ##     /   ###/  ##/   ### \n");
    printf("##   ##     ##         ### /##     ##       ##           ##    ##         ##     ## \n");
    printf("##   ##     ##           #/ /##    ##       ##           ##    ##         ##     ## \n");
    printf(" ##  ##     ##            #/ ##    ##       ##           ##    ##         ##     ## \n");
    printf("  ## #      /              # /     ##       ##           ##    ##         ##     ## \n");
    printf("   ###     /     /##        /      ##       ##           ##    ###     /  ##     ## \n");
    printf("    ######/     /  ########/       ##       ###          ### /  ######/   ##     ## \n");
    printf("      ###      /     #####          ##       ###          ##/    #####     ##    ## \n");
    printf("               |                                                                 /  \n");
    printf("                \\)                                                              /   \n");
    printf("                                                                               /    \n");
    printf("                                                                              /     \n");
}

void test_function (int a) {
    while (1) {
        for (int i = 0; i < 5; i++) {
            printf("%d ", a + i);
            delay(10000000);
        }
    }
}

extern "C" void kernel_init() {
    if(getCoreID() == 0){
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        timer_init();
        enable_interrupt_controller();
        enable_irq();
        printf("printf initialized!!!\n");
        // print_ascii_art();
        // Initialize MMU page tables
        // Initialize heap
        smpInitDone = true;
        wake_up_cores();
    }

    // Enable MMU (all cores must set the enable MMU bit to 1)

    // this line here will cause race conditions between cores
    printf("Hi, I'm core %d\n", getCoreID());
    if(getCoreID() == 0){
        int res = copy_process((unsigned long)&test_function, 10);
        if (res != 0) {
            printf("error while starting process 1");
            return;
        }
        res = copy_process((unsigned long)&test_function, 20);
        if (res != 0) {
            printf("error while starting process 2");
            return;
        }
        res = copy_process((unsigned long)&test_function, 40);
        if (res != 0) {
            printf("error while starting process 3");
            return;
        }
        while (1) {
            schedule();
        }
    }
}


