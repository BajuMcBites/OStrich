#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "irq.h"
#include "timer.h"
#include "entry.h"
#include "core.h"
#include "sched.h"
#include "mm.h"
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
        for (int i = 0; i < a; i++) {
            printf("%d ", a + i);
        }
    }
}

extern "C" int copy_process(unsigned long fn, unsigned long arg)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *) get_free_page();
    if (!p)
        return 1;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.x19 = fn;
    p->cpu_context.x20 = arg;
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
    int pid = nr_tasks++;
    task[pid] = p;
    preempt_enable();
    return 0;
}

extern "C" void kernel_init() {
    if(getCoreID() == 0){
        uart_init();
        irq_vector_init();
        timer_init();
        enable_interrupt_controller();
        enable_irq();
        init_printf(nullptr, uart_putc_wrapper);
        printf("printf initialized!!!\n");
        print_ascii_art();
        // Initialize MMU page tables
        // Initialize heap
        smpInitDone = true;
        //wake_up_cores();
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
        while (1) {
            printf("ASDASDSA\n");
            schedule();
            //uart_putc(uart_getc()); // will allow you to type letters through UART
        }
    }
}


