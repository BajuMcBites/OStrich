#include "core.h"
#include "dcache.h"
#include "dwc.h"
#include "event.h"
#include "fcs.h"
#include "fork.h"
#include "frame.h"
#include "framebuffer.h"
#include "fs_init.h"
#include "heap.h"
#include "irq.h"
#include "kernel_tests.h"
#include "keyboard.h"
#include "libk.h"
#include "listener.h"
#include "mm.h"
#include "partition.h"
#include "partition_tests.h"
#include "percpu.h"
#include "printf.h"
#include "queue.h"
#include "ramfs.h"
#include "sched.h"
#include "sdio.h"
#include "sdio_tests.h"
#include "snake.h"
#include "stdint.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "vm.h"

void mergeCores();

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
    // printf("All tests passed\n");
    heapTests();
    event_loop_tests();
    hash_test();
    frame_alloc_tests();
    // user_paging_tests();
    blocking_atomic_tests();
    // ramfs_tests();
    sdioTests();
    ring_buffer_tests();
    elf_load_test();
    partitionTests();
    // test_fs();
    // testSnapshot();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

static Atomic<int> coresAwake(0);

void mergeCores();

extern "C" void secondary_kernel_init() {
    init_mmu();
    event_listener_init();
    mergeCores();
}

extern "C" void primary_kernel_init() {
    create_page_tables();
    patch_page_tables();
    init_mmu();
    uart_init();
    init_printf(nullptr, uart_putc_wrapper);
    // timer_init();
    // enable_interrupt_controller();
    // enable_irq();
    printf("printf initialized!!!\n");
    if (fb_init()) {
        printf("Framebuffer initialization successful!\n");
    } else {
        printf("Framebuffer initialization failed!\n");
    }

    if (sd_init() == 0) {
        printf("SD card initialized successfully!\n");
    } else {
        printf("SD card initialization failed!\n");
    }
    // The Alignment check enable bit in the SCTLR_EL1 register will make an error ocurr here.
    // making that bit making that bit 0 will allow ramfs to be initalized. (will get ESR_EL1 value
    // of 0x96000021)
    init_ramfs();
    create_frame_table(frame_table_start,
                       0x40000000);  // assuming 1GB memory (Raspberry Pi 3b)
    printf("frame table initialized! \n");
    uinit((void *)HEAP_START, HEAP_SIZE);
    event_listener_init();

    usb_initialize();

    set_partition_table(1024 * 1024 /* Filesystem (Bytes) */, 1024 * 1024 /* Swap (Bytes) */);
    fs_init();

    smpInitDone = true;
    // with data cache on, we must write the boolean back to memory to allow other cores to see it.
    clean_dcache_line(&smpInitDone);
    init_page_cache();
    wake_up_cores();
    mergeCores();
}

void mergeCores() {
    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);
    K::check_stack();

    if (number_awake == CORE_COUNT) {
        create_event([] { kernel_main(); });
    }

    // Uncomment to run snake
    // if(getCoreID() == 0){
    //     printf("init_snake() + keyboard_loop();\n");
    //     create_event(init_snake);
    //     create_event(keyboard_loop);
    // }
    event_loop();
    printf("PANIC I should not go here\n");
}