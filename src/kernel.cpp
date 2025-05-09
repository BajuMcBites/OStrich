#include "core.h"
#include "dcache.h"
#include "dwc.h"
#include "event.h"
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
#include "peripherals/arm_devices.h"
#include "printf.h"
#include "queue.h"
#include "ramfs.h"
#include "sdio.h"
#include "sdio_tests.h"
#include "snake.h"
#include "stdint.h"
#include "string.h"
#include "timer.h"
#include "tty.h"
#include "uart.h"
#include "utils.h"
#include "vm.h"

void mergeCores();

struct __attribute__((__packed__, aligned(4))) SystemTimerRegisters {
    uint32_t ControlStatus;  // 0x00
    uint32_t TimerLo;        // 0x04
    uint32_t TimerHi;        // 0x08
    uint32_t Compare0;       // 0x0C
    uint32_t Compare1;       // 0x10
    uint32_t Compare2;       // 0x14
    uint32_t Compare3;       // 0x18
};

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
    // stringTest();

    printf("All tests passed\n");
    // heapTests();
    // event_loop_tests();
    // hash_test();
    // frame_alloc_tests();
    // user_paging_tests();
    // blocking_atomic_tests();
    // ramfs_tests();
    // sdioTests();
    // ring_buffer_tests();
    elf_load_test();
    // partitionTests();
    // stringTest();

    // // filesystem
    // // test_fs();
    // // testSnapshot();
    // // test_fs_requests();

    // kernel file interface
    // kfs_simple_test();
    // kfs_kopen_uses_cache_test();
    // kfs_stress_test(10);
    // sd_stress_test();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

static Atomic<int> coresAwake(0);

static Barrier* starting = nullptr;
static Barrier* stopping = nullptr;

void mergeCores();

extern "C" void secondary_kernel_init() {
    init_mmu();
    enable_irq();
    event_listener_init();
    // while (1);
    mergeCores();
}

extern "C" void primary_kernel_init() {
    create_page_tables();
    patch_page_tables();
    init_mmu();
    uart_init();
    init_printf(nullptr, uart_putc_wrapper);
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

    constexpr int FILESYSTEM_SIZE_MB = 4;
    constexpr int SWAP_SIZE_MB = 4;

    // Sector 0 is the partition table, so we really have SD_SIZE - 1 * SD_BLK_SIZE bytes available.
    // But also, swap and filesystem operate at a page granularity, so we need to subtract a
    // multiple of 4096 from one of them. I chose to subtract 256KB from the swap partition (also
    // need extra space for partition tests).
    set_partition_table(FILESYSTEM_SIZE_MB * 1024 * 1024, SWAP_SIZE_MB * 1024 * 1024 - 256 * 1024);
    fs_init();

    smpInitDone = true;
    // with data cache on, we must write the boolean back to memory to allow other cores to see it.
    clean_dcache_line(&smpInitDone);
    init_page_cache();
    init_dummy_tcb();  // MUST COME BEFORE LOCAL TIMER INIT
    local_timer_init();
    init_swap();
    init_tty();

    starting = new Barrier(CORE_COUNT);
    stopping = new Barrier(CORE_COUNT);


    wake_up_cores();
    enable_irq();
    mergeCores();
}

void mergeCores() {
    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);
    K::check_stack();
    starting->sync();
    if (number_awake == CORE_COUNT) {
        printf("creating kernel_main\n");
        create_event(keyboard_loop);
        create_event([] { kernel_main(); });
        // elf_load_test();
    }
    // Uncomment to run snake
    if (getCoreID() == 0) {
        printf("init_snake() + keyboard_loop();\n");
        create_event(run_tty);
        create_event([] {
            auto tcb = get_running_task(getCoreID());
            tcb->frameBuffer = request_tty();  // give the snake tcb its own frame buffer
            init_snake();
        });

        create_event([] {
            init_animation();
            update_animation();
            create_event(update_animation);
            create_event(update_animation);
            create_event(update_animation);
            create_event(update_animation);
        });

        create_event(run_tty);
    }
    event_loop();
    stopping->sync();
    printf("PANIC I should not go here\n");
}
