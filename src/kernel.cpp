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

struct __attribute__((__packed__, aligned(4))) SystemTimerRegisters {
    uint32_t ControlStatus;  // 0x00
    uint32_t TimerLo;        // 0x04
    uint32_t TimerHi;        // 0x08
    uint32_t Compare0;       // 0x0C
    uint32_t Compare1;       // 0x10
    uint32_t Compare2;       // 0x14
    uint32_t Compare3;       // 0x18
};

#define SYSTEMTIMER \
    ((volatile      \
      __attribute__((aligned(4))) struct SystemTimerRegisters *)(uintptr_t)(PBASE + 0x3000))

uint64_t timer_getTickCount64(void) {
    uint64_t resVal;
    uint32_t lowCount;
    do {
        resVal = SYSTEMTIMER->TimerHi;    // Read Arm system timer high count
        lowCount = SYSTEMTIMER->TimerLo;  // Read Arm system timer low count
    } while (resVal !=
             (uint64_t)SYSTEMTIMER->TimerHi);  // Check hi counter hasn't rolled in that time
    resVal = (uint64_t)resVal << 32 | lowCount;  // Join the 32 bit values to a full 64 bit
    return (resVal);                             // Return the uint64_t timer tick count
}

void timer_wait(uint64_t us) {
    us += timer_getTickCount64();  // Add current tickcount onto delay
    while (timer_getTickCount64() < us) {
    };  // Loop on timeout function until timeout
}

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
    // heapTests();
    // event_loop_tests();
    // hash_test();
    // frame_alloc_tests();
    // user_paging_tests();
    // blocking_atomic_tests();
    // ramfs_tests();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

static Atomic<int> coresAwake(0);

Atomic<int> curr_interrupt(0);

extern "C" void kernel_init() {
    if (getCoreID() == 0) {
        create_page_tables();
        patch_page_tables();
        init_mmu();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
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
        enable_irq();
        
        curr_interrupt = 0;

        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ;  // Start with Core 0
        QA7->TimerControlStatus.ReloadValue = 2000000;        // Set timer interval (e.g., 2M cycles)
        QA7->TimerControlStatus.TimerEnable = 1;              // Enable timer
        QA7->TimerControlStatus.IntEnable = 1;                // Enable interrupts
        QA7->TimerClearReload.Raw32 = (1 << 31) | (1 << 30);  // Clear + Reload


        // printf("TimerRouting: 0x%x\n", QA7->TimerRouting.Raw32);
        // printf("Setting up Local Timer Irq to Core0\n");
        // QA7->TimerRouting.Routing =
        //     LOCALTIMER_TO_CORE0_IRQ;  // Route local timer IRQ to Core0 // Ensure the
        // printf("TimerRouting: 0x%x\n", QA7->TimerRouting.Raw32);
        // printf("TimerRouting adr:0x%x\n", (void *)&QA7->TimerRouting.Raw32);

        // printf("TimerControlStatus: 0x%x\n", QA7->TimerControlStatus.Raw32);
        // QA7->TimerControlStatus.ReloadValue = 500000;  // Timer period set
        // QA7->TimerControlStatus.TimerEnable = 1;       // Timer enabled
        // QA7->TimerControlStatus.IntEnable = 1;         // Timer IRQ enabled
        // printf("TimerControlStatus: 0x%x\n", QA7->TimerControlStatus.Raw32);
        // printf("TimerControlStatus ard: 0x%x\n", (void *)&QA7->TimerControlStatus.Raw32);

        // printf("TimerClearReload: 0x%x\n", QA7->TimerClearReload.Raw32);
        // QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
        // QA7->TimerClearReload.Reload = 1;    // Reload now
        // printf("TimerClearReload: 0x%x\n",
        //        QA7->TimerClearReload.Raw32);  // This should read 0 but is not??
        // printf("TimerClearReload adr: 0x%x\n", (void *)&QA7->TimerClearReload.Raw32);

        // printf("Core3TimerIntControl: 0x%x\n", QA7->Core0TimerIntControl.Raw32);
        // QA7->Core0TimerIntControl.nCNTPNSIRQ_IRQ =
        //     1;  // We are in NS EL1 so enable IRQ to core0 that level
        // QA7->Core0TimerIntControl.nCNTPNSIRQ_FIQ = 0;  // Make sure FIQ is zero
        // printf("Core0TimerIntControl: 0x%x\n", QA7->Core0TimerIntControl.Raw32);
        // printf("Core0TimerIntControl adr: 0x%x\n", (void *)&QA7->Core0TimerIntControl.Raw32);
        wake_up_cores();
        enable_interrupt_controller();
        // printf("waking\n");
        //  kernel_main();
    } else {
        init_mmu();
        enable_irq();
        init_core_timer();
        // while (1) {
        //     // printf("Hi, I'm core %d\n", getCoreID());
        // }
    }

    printf("Hi, I'm core %d\n", getCoreID());
    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);
    //  K::check_stack();

    if (number_awake == CORE_COUNT) {
        create_event([] { kernel_main(); });
        while (1) {
            printf("timer tick %d\n", timer_getTickCount64());
            timer_wait(500000);
        }
        // user_thread([]
        //             { printf("i do nothing2\n"); });
    }
    stop();
    printf("PANIC I should not go here\n");
}
