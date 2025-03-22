#include "core.h"
#include "dwc.h"
#include "event.h"
#include "fork.h"
#include "frame.h"
#include "heap.h"
#include "hid.h"
#include "irq.h"
#include "kernel_tests.h"
#include "libk.h"
#include "mm.h"
#include "percpu.h"
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
    static constexpr int BYTES = 16384;
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

    // Initialize mouse driver
    for (int i = 0; i < 1; i++) {
        printf("interval: %d\n", USB_MOUSE.interval_ms);
        usleep(USB_MOUSE.interval_ms);
        mouse_process_input(&USB_MOUSE);
    }

    printf("done\n");

    // heapTests();
    // event_loop_tests();
    // hash_test();
    // frame_alloc_tests();
    // user_paging_tests();
    // ramfs_tests();
    // ip_tests();
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

        // Initialize USB.
        // If we have a mouse, it will be initialized in the USB driver setup.

        // timer_init();
        // enable_interrupt_controller();
        // enable_irq();

        printf("printf initialized!!!\n");
        usb_initialize();

        init_ramfs();
        create_frame_table(frame_table_start,
                           0x40000000);  // assuming 1GB memory (Raspberry Pi 3b)
        breakpoint();
        print_ascii_art();
        uinit((void *)HEAP_START, HEAP_SIZE);
        smpInitDone = true;
        threadsInit();
        wake_up_cores();
    } else {
        init_mmu();
    }

    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);
    K::check_stack();

    if (number_awake == CORE_COUNT) {
        create_event([] { kernel_main(); });
    }
    stop();
    printf("PANIC I should not go here\n");
}
