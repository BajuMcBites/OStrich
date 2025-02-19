#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "stdint.h"
#include "irq.h"
#include "timer.h"
#include "core.h"
#include "sched.h"
#include "mm.h"
#include "fork.h"
#include "vm.h"
#include "mm.h"
#include "heap.h"
#include "libk.h"
#include "kernel_tests.h"
#include "queue.h"
#include "event_loop.h"

struct Stack
{
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void)
{
    return (uint64_t)&stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void print_ascii_art()
{
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

void breakpoint()
{
    return;
}

extern "C" void kernel_main()
{
    heapTests();
    event_loop_tests();
    queue_test();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

extern "C" void kernel_init()
{
    if (getCoreID() == 0)
    {
        create_page_tables();
        init_mmu();
        patch_page_tables();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        timer_init();
        enable_interrupt_controller();
        enable_irq();
        printf("printf initialized!!!\n");
        breakpoint();
        print_ascii_art();
        uinit((void *)HEAP_START, HEAP_SIZE);

        // event queues setup
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < MAX_PRIORITY; j++) {
                void* q_addr = malloc(sizeof(queue<event*>));
                cpu_queues.forCPU(i).queue_list[j] = new (q_addr) queue<event*>(); // using placement new
            }
        }

        smpInitDone = true;
        wake_up_cores();
        kernel_main();
    }
    else
    {
        init_mmu();
    }

    printf("Hi, I'm core %d\n", getCoreID());

    if(getCoreID() == 0){
        while (1) {
            loop();
        }
    }
}

