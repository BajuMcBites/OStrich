#include "core.h"
#include "event.h"
#include "fork.h"
#include "frame.h"
#include "heap.h"
#include "irq.h"
#include "kernel_tests.h"
#include "libk.h"
#include "mm.h"
#include "percpu.h"
#include "printf.h"
#include "queue.h"
#include "sched.h"
#include "stdint.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "vm.h"

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void) {
    return (uint64_t)&stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void print_ascii_art() {
    // clang-format off
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
    // clang-format on
}

void breakpoint() {
    return;
}

extern char _frame_table_start[];

#define frame_table_start ((uintptr_t)_frame_table_start)

extern "C" void kernel_main() {
    heapTests();
    event_loop_tests();
    // queue_test();
    frame_alloc_tests();
    user_paging_tests();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

static Atomic<int> coresAwake(0);

extern "C" void kernel_init() {
    if (getCoreID() == 0) {
        create_page_tables();
        init_mmu();
        patch_page_tables();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        timer_init();
        enable_interrupt_controller();
        enable_irq();
        printf("printf initialized!!!\n");
        create_frame_table(frame_table_start,
                           0x40000000);  // assuming 1GB memory (Raspberry Pi 3b)
        printf("frame table initialized! \n");
        breakpoint();
        print_ascii_art();
        uinit((void *)HEAP_START, HEAP_SIZE);
        smpInitDone = true;
        threadsInit();
        wake_up_cores();
        //  kernel_main();
    } else {
        init_mmu();
    }

    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);

    if (number_awake == CORE_COUNT) {
        create_event([] { kernel_main(); });

        // user_thread([]
        //             { printf("i do nothing2\n"); });
    }
    stop();
    printf("PANIC I should not go here\n");
}
