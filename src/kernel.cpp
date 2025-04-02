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
#include "peripherals/arm_devices.h"
#include "printf.h"
#include "queue.h"
#include "ramfs.h"
#include "sched.h"
#include "stdint.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "vm.h"

struct Stack {
    static constexpr int BYTES = CORE_STACK_SIZE;
    uint64_t bytes[BYTES] __attribute__((aligned(16)));
};

PerCPU<Stack> stacks __attribute__((section(".stacks")));

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
    // queue_test();
    printf("All tests passed\n");
    heapTests();
    event_loop_tests();
    hash_test();
    frame_alloc_tests();
    user_paging_tests();
    blocking_atomic_tests();
    ramfs_tests();
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
        gic_init();
        // timer_init();
        core_timer_init();
        enable_interrupt_controller();

        enable_irq();
        printf("printf initialized!!!\n");
        init_ramfs();
        create_frame_table(frame_table_start,
                           0x40000000);  // assuming 1GB memory (Raspberry Pi 3b)
        printf("frame table initialized! \n");
        breakpoint();
        print_ascii_art();
        uinit((void *)HEAP_START, HEAP_SIZE);
        smpInitDone = true;
        threadsInit();

        printf("Setting up Local Timer Irq to Core3\n");

        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE3_IRQ;  // Route local timer IRQ to Core0

        QA7->TimerControlStatus.ReloadValue = 20000000;  // Timer period set
        QA7->TimerControlStatus.TimerEnable = 1;         // Timer enabled
        QA7->TimerControlStatus.IntEnable = 1;           // Timer IRQ enabled

        QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
        QA7->TimerClearReload.Reload = 1;    // Reload now

        printf("reload addr %x\n", QA7->TimerControlStatus);
        QA7->Core3TimerIntControl.nCNTPNSIRQ_IRQ =
            1;  // We are in NS EL1 so enable IRQ to core0 that level
        QA7->Core3TimerIntControl.nCNTPNSIRQ_FIQ = 0;  // Make sure FIQ is zero

        wake_up_cores();
        //  kernel_main();
    } else {
        init_mmu();
        // core_timer_init();
        // timer_init();
        // enable_interrupt_controller();
        enable_irq();
    }

    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);
    //  K::check_stack();

    if (number_awake == CORE_COUNT) {
        create_event([] { kernel_main(); });

        // user_thread([]
        //             { printf("i do nothing2\n"); });
    }
    stop();
    printf("PANIC I should not go here\n");
}
